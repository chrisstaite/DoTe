
#include "forwarder_connection.h"
#include "i_loop.h"
#include "i_forwarder_config.h"
#include "openssl/i_ssl_factory.h"
#include "openssl/spki_verifier.h"
#include "socket.h"
#include "log.h"

namespace dote {

using namespace std::placeholders;

ForwarderConnection::ForwarderConnection(std::shared_ptr<ILoop> loop,
                                         std::shared_ptr<IForwarderConfig> config,
                                         std::shared_ptr<openssl::ISslFactory> ssl) :
    m_timeout(time(nullptr) + config->timeout()),
    m_loop(std::move(loop)),
    m_config(std::move(config)),
    m_connection(ssl->create()),
    m_state(CONNECTING),
    m_socket(nullptr)
{
    auto chosen = m_config->get();
    if (m_connection && chosen != m_config->end())
    {
        m_forwarder = *chosen;

        configureVerifier();

        m_socket = Socket::connect(m_forwarder.remote, Socket::Type::TCP);

        if (m_socket)
        {
            m_connection->setSocket(m_socket->get());
            m_exception = m_loop->registerException(
                m_socket->get(),
                std::bind(&ForwarderConnection::exception, this, _1)
            );
            connect(m_socket->get());
        }
        else
        {
            m_config->setBad(m_forwarder);
            m_state = CLOSED;
        }
    }
    else
    {
        m_state = CLOSED;
    }
}

ForwarderConnection::~ForwarderConnection()
{
    close();
}

void ForwarderConnection::configureVerifier()
{
    if (m_forwarder.disablePki)
    {
        m_connection->disableVerification();
    }
    else if (!m_forwarder.host.empty() || !m_forwarder.pin.empty())
    {
        openssl::SpkiVerifier verifier(m_forwarder);
        m_connection->setVerifier([verifier](X509_STORE_CTX* store) {
            return verifier.verify(store);
        });
    }
}

void ForwarderConnection::setIncomingCallback(IncomingCallback incoming)
{
    m_incoming = std::move(incoming);
}

void ForwarderConnection::setShutdownCallback(ShutdownCallback shutdown)
{
    m_shutdown = std::move(shutdown);
}

bool ForwarderConnection::closed()
{
    return (m_state == SHUTTING_DOWN || m_state == CLOSED);
}

void ForwarderConnection::connect(int handle)
{
    switch (m_connection->connect())
    {
        case openssl::SslConnection::Result::NEED_READ:
            if (!m_read)
            {
                m_read = m_loop->registerRead(
                    m_socket->get(),
                    std::bind(&ForwarderConnection::connect, this, _1),
                    m_timeout
                );
            }
            m_write.reset();
            break;
        case openssl::SslConnection::Result::NEED_WRITE:
            if (!m_write)
            {
                m_write = m_loop->registerWrite(
                    m_socket->get(),
                    std::bind(&ForwarderConnection::connect, this, _1),
                    m_timeout
                );
            }
            m_read.reset();
            break;
        case openssl::SslConnection::Result::SUCCESS:
            // Remove the handlers to add the running ones.
            m_read.reset();
            m_write.reset();
            m_read = m_loop->registerRead(
                m_socket->get(),
                std::bind(&ForwarderConnection::incoming, this, _1),
                m_timeout
            );
            if (!m_buffer.empty())
            {
                m_write = m_loop->registerWrite(
                    m_socket->get(),
                    std::bind(&ForwarderConnection::outgoing, this, _1),
                    m_timeout
                );
            }
            m_state = State::OPEN;
            break;
        case openssl::SslConnection::Result::FATAL:
            Log::notice << "Error handshaking with forwarder";
            m_config->setBad(m_forwarder);
            // Fall through to closed
        case openssl::SslConnection::Result::CLOSED:
            close();
            break;
    }
}

void ForwarderConnection::incoming(int handle)
{
    std::vector<char> buffer;
    switch (m_connection->read(buffer))
    {
        case openssl::SslConnection::Result::NEED_READ:
            // Nothing required to do, we're always the read handler
            break;
        case openssl::SslConnection::Result::NEED_WRITE:
            // Probably will be fine if we ignore this
            break;
        case openssl::SslConnection::Result::SUCCESS:
            if (!buffer.empty() && m_incoming)
            {
                m_incoming(*this, std::move(buffer));
            }
            break;
        case openssl::SslConnection::Result::FATAL:
            Log::notice << "Error reading from forwarder";
            m_config->setBad(m_forwarder);
            // Fall through to closed
        case openssl::SslConnection::Result::CLOSED:
            close();
            break;
    }
}

void ForwarderConnection::shutdown()
{
    if (m_state == CONNECTING || m_state == OPEN)
    {
        m_read.reset();
        m_write.reset();
        _shutdown(m_socket->get());
    }
}

void ForwarderConnection::_shutdown(int handle)
{
    m_state = State::SHUTTING_DOWN;

    switch (m_connection->shutdown())
    {
        case openssl::SslConnection::Result::NEED_READ:
            if (!m_read) {
                m_read = m_loop->registerRead(
                    m_socket->get(),
                    std::bind(&ForwarderConnection::_shutdown, this, _1),
                    m_timeout
                );
            }
            m_write.reset();
            break;
        case openssl::SslConnection::Result::NEED_WRITE:
            if (!m_write)
            {
                m_write = m_loop->registerWrite(
                    m_socket->get(),
                    std::bind(&ForwarderConnection::_shutdown, this, _1),
                    m_timeout
                );
            }
            m_read.reset();
            break;
        case openssl::SslConnection::Result::CLOSED:
            // Fall through
        case openssl::SslConnection::Result::SUCCESS:
            // Fall through
        case openssl::SslConnection::Result::FATAL:
            close();
            break;
    }
}

bool ForwarderConnection::send(std::vector<char> buffer)
{
    if (!m_socket || (m_state != State::CONNECTING && m_state != State::OPEN))
    {
        return false;
    }

    if (m_buffer.empty())
    {
        if (m_state == State::OPEN && !m_write)
        {
            m_write = m_loop->registerWrite(
                m_socket->get(),
                std::bind(&ForwarderConnection::outgoing, this, _1),
                m_timeout
            );
        }
        m_buffer = std::move(buffer);
        return true;
    }

    return false;
}

void ForwarderConnection::outgoing(int handle)
{
    switch (m_connection->write(m_buffer))
    {
        case openssl::SslConnection::Result::NEED_READ:
            // Probably fine to ignore like the incoming
            break;
        case openssl::SslConnection::Result::NEED_WRITE:
            // Nothing required to do, we're always the write handler
            break;
        case openssl::SslConnection::Result::SUCCESS:
            m_buffer.clear();
            m_write.reset();
            break;
        case openssl::SslConnection::Result::FATAL:
            Log::notice << "Error writing to forwarder";
            m_config->setBad(m_forwarder);
            // Fall through to closed
        case openssl::SslConnection::Result::CLOSED:
            close();
            break;
    }
}

void ForwarderConnection::exception(int handle)
{
    if (m_state == State::CONNECTING)
    {
        Log::notice << "Issue connecting to forwarder";
        m_config->setBad(m_forwarder);
    }
    close();
}

void ForwarderConnection::close()
{
    if (m_socket)
    {
        int handle = m_socket->get();
        m_read.reset();
        m_write.reset();
        m_exception.reset();
        m_state = State::CLOSED;
        m_socket.reset();

        if (m_shutdown)
        {
            m_shutdown(*this);
        }
    }
}

}  // namespace dote
