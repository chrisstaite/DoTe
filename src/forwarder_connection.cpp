
#include "forwarder_connection.h"
#include "loop.h"
#include "forwarder_config.h"
#include "socket.h"

namespace dote {

using namespace std::placeholders;

ForwarderConnection::ForwarderConnection(std::shared_ptr<Loop> loop,
                                         std::shared_ptr<ForwarderConfig> config,
                                         IncomingCallback incoming,
                                         ShutdownCallback shutdown,
                                         std::shared_ptr<openssl::Context> context) :
    m_loop(std::move(loop)),
    m_config(std::move(config)),
    m_incoming(std::move(incoming)),
    m_shutdown(std::move(shutdown)),
    m_connection(std::move(context)),
    m_state(CONNECTING),
    m_socket(nullptr),
    m_buffers(),
    m_pin()
{
    auto chosen = m_config->get();
    if (chosen != m_config->end())
    {
        m_socket = Socket::connect(chosen->remote, Socket::Type::TCP);
        m_hostname = chosen->host;
        m_pin = chosen->pin;

        if (m_socket)
        {
            m_connection.setSocket(m_socket->get());
            m_loop->registerException(
                m_socket->get(),
                std::bind(&ForwarderConnection::exception, this, _1)
            );
            connect(m_socket->get());
        }
    }
}

ForwarderConnection::~ForwarderConnection()
{
    close();
}

bool ForwarderConnection::closed()
{
    return (m_state == SHUTTING_DOWN || m_state == CLOSED);
}

bool ForwarderConnection::verifyConnection()
{
    return m_connection.verifyHostname(m_hostname) &&
        (m_pin.empty() || m_connection.getPeerCertificateHash() == m_pin);
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
                if (!m_buffers.empty())
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
                shutdown();
            }
            break;
        case openssl::SslConnection::Result::FATAL:
            m_state = State::CLOSED;
            m_loop->removeException(handle);
            m_socket.reset();
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
                m_incoming(*this, std::move(buffer));
            }
            break;
        case openssl::SslConnection::Result::FATAL:
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
            break;
    }
}

bool ForwarderConnection::send(std::vector<char> buffer)
{
    if (!m_socket || (m_state != State::CONNECTING && m_state != State::OPEN))
    {
        return false;
    }

    if (m_buffers.empty())
    {
        m_loop->registerWrite(
            m_socket->get(),
            std::bind(&ForwarderConnection::outgoing, this, _1)
        );
        m_buffers.emplace_back(std::move(buffer));
        outgoing(m_socket->get());
    }
    else
    {
        m_buffers.emplace_back(std::move(buffer));
    }

    return (m_state == State::OPEN);
}

void ForwarderConnection::outgoing(int handle)
{
    switch (m_connection.write(m_buffers.front()))
    {
        case openssl::SslConnection::Result::NEED_READ:
            // Probably fine to ignore like the incoming
            break;
        case openssl::SslConnection::Result::NEED_WRITE:
            // Nothing required to do, we're always the write handler
            break;
        case openssl::SslConnection::Result::SUCCESS:
            m_buffers.pop_front();
            if (m_buffers.empty())
            {
                m_loop->removeWrite(handle);
            }
            break;
        case openssl::SslConnection::Result::FATAL:
            close();
            break;
    }
}

void ForwarderConnection::exception(int handle)
{
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

        m_shutdown(*this);
    }
}

}  // namespace dote
