
#include "forwarder_connection.h"
#include "i_loop.h"
#include "i_forwarder_config.h"
#include "socket.h"
#include "log.h"

namespace dote {

using namespace std::placeholders;

ForwarderConnection::ForwarderConnection(std::shared_ptr<ILoop> loop,
                                         std::shared_ptr<IForwarderConfig> config,
                                         std::shared_ptr<openssl::Context> context) :
    m_loop(std::move(loop)),
    m_config(std::move(config)),
    m_connection(std::move(context)),
    m_incoming(),
    m_shutdown(),
    m_state(CONNECTING),
    m_socket(nullptr),
    m_buffer(),
    m_forwarder()
{
    auto chosen = m_config->get();
    if (chosen != m_config->end())
    {
        m_forwarder = *chosen;
        m_socket = Socket::connect(m_forwarder.remote, Socket::Type::TCP);

        if (m_socket)
        {
            m_connection.setSocket(m_socket->get());
            m_loop->registerException(
                m_socket->get(),
                std::bind(&ForwarderConnection::exception, this, _1)
            );
            connect(m_socket->get());
        }
        else
        {
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

bool ForwarderConnection::verifyConnection()
{
    return m_connection.verifyHostname(m_forwarder.host) &&
        (m_forwarder.pin.empty() ||
            m_connection.getPeerCertificateHash() == m_forwarder.pin);
}

void ForwarderConnection::connect(int handle)
{
    m_loop->removeWrite(handle);
    m_loop->removeRead(handle);

    switch (m_connection.connect())
    {
        case openssl::SslConnection::Result::NEED_READ:
            m_loop->registerRead(
                m_socket->get(),
                std::bind(&ForwarderConnection::connect, this, _1)
            );
            break;
        case openssl::SslConnection::Result::NEED_WRITE:
            m_loop->registerWrite(
                m_socket->get(),
                std::bind(&ForwarderConnection::connect, this, _1)
            );
            break;
        case openssl::SslConnection::Result::SUCCESS:
            if (verifyConnection())
            {
                m_loop->registerRead(
                    m_socket->get(),
                    std::bind(&ForwarderConnection::incoming, this, _1)
                );
                if (!m_buffer.empty())
                {
                    m_loop->registerWrite(
                        m_socket->get(),
                        std::bind(&ForwarderConnection::outgoing, this, _1)
                    );
                }
                m_state = State::OPEN;
            }
            else
            {
                Log::notice << "Bad hostname or certificate for forwarder";
                m_config->setBad(m_forwarder);
                shutdown();
            }
            break;
        case openssl::SslConnection::Result::FATAL:
            Log::notice << "Error handshaking with forwarder";
            m_config->setBad(m_forwarder);
            m_state = State::CLOSED;
            m_loop->removeException(handle);
            m_socket.reset();
            if (m_shutdown)
            {
                m_shutdown(*this);
            }
            break;
    }
}

void ForwarderConnection::incoming(int handle)
{
    std::vector<char> buffer;
    switch (m_connection.read(buffer))
    {
        case openssl::SslConnection::Result::NEED_READ:
            // Nothing required to do, we're always the read handler
            break;
        case openssl::SslConnection::Result::NEED_WRITE:
            // Probably will be fine if we ignore this
            break;
        case openssl::SslConnection::Result::SUCCESS:
            if (!buffer.empty())
            {
                if (m_incoming)
                {
                    m_incoming(*this, std::move(buffer));
                }
            }
            break;
        case openssl::SslConnection::Result::FATAL:
            Log::notice << "Error reading from forwarder";
            m_config->setBad(m_forwarder);
            close();
            break;
    }
}

void ForwarderConnection::shutdown()
{
    if (m_state == CONNECTING || m_state == OPEN)
    {
        _shutdown(m_socket->get());
    }
}

void ForwarderConnection::_shutdown(int handle)
{
    m_loop->removeWrite(handle);
    m_loop->removeRead(handle);

    m_state = State::SHUTTING_DOWN;

    switch (m_connection.shutdown())
    {
        case openssl::SslConnection::Result::NEED_READ:
            m_loop->registerRead(
                m_socket->get(),
                std::bind(&ForwarderConnection::_shutdown, this, _1)
            );
            break;
        case openssl::SslConnection::Result::NEED_WRITE:
            m_loop->registerWrite(
                m_socket->get(),
                std::bind(&ForwarderConnection::_shutdown, this, _1)
            );
            break;
        case openssl::SslConnection::Result::SUCCESS:
            // Fall through
        case openssl::SslConnection::Result::FATAL:
            m_state = State::CLOSED;
            m_loop->removeException(handle);
            m_socket.reset();
            if (m_shutdown)
            {
                m_shutdown(*this);
            }
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
        if (m_state == State::OPEN)
        {
            m_loop->registerWrite(
                m_socket->get(),
                std::bind(&ForwarderConnection::outgoing, this, _1)
            );
        }
        m_buffer = std::move(buffer);
        return true;
    }

    return false;
}

void ForwarderConnection::outgoing(int handle)
{
    switch (m_connection.write(m_buffer))
    {
        case openssl::SslConnection::Result::NEED_READ:
            // Probably fine to ignore like the incoming
            break;
        case openssl::SslConnection::Result::NEED_WRITE:
            // Nothing required to do, we're always the write handler
            break;
        case openssl::SslConnection::Result::SUCCESS:
            m_buffer.clear();
            m_loop->removeWrite(handle);
            break;
        case openssl::SslConnection::Result::FATAL:
            Log::notice << "Error writing to forwarder";
            m_config->setBad(m_forwarder);
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
        m_loop->removeRead(handle);
        m_loop->removeWrite(handle);
        m_loop->removeException(handle);
        m_state = State::CLOSED;
        m_socket.reset();

        if (m_shutdown)
        {
            m_shutdown(*this);
        }
    }
}

}  // namespace dote
