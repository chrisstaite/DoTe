
#include "ip_lookup.h"
#include "config_parser.h"
#include "loop.h"
#include "socket.h"
#include "openssl/ssl_connection.h"
#include "openssl/context.h"
#include "openssl/base64.h"

namespace dote {

using namespace std::placeholders;

IpLookup::IpLookup(const ConfigParser& config) :
    m_loop(std::make_shared<Loop>()),
    m_socket(Socket::connect(config.ipLookup(), Socket::Type::TCP)),
    m_connection(nullptr),
    m_timeout(time(nullptr) + config.timeout())
{
    std::shared_ptr<openssl::Context> context(
        std::make_shared<openssl::Context>(config.ciphers())
    );
    m_connection = std::make_shared<openssl::SslConnection>(context);
    m_connection->setSocket(m_socket->get());
    m_loop->registerException(
        m_socket->get(),
        std::bind(&IpLookup::connect, this, _1)
    );
    connect(m_socket->get());

    // Run until completion
    m_loop->run();
}

IpLookup::~IpLookup()
{ }

void IpLookup::connect(int handle)
{
    m_loop->removeWrite(handle);
    m_loop->removeRead(handle);

    switch (m_connection->connect())
    {
        case openssl::SslConnection::Result::NEED_READ:
            m_loop->registerRead(
                m_socket->get(),
                std::bind(&IpLookup::connect, this, _1),
                m_timeout
            );
            break;
        case openssl::SslConnection::Result::NEED_WRITE:
            m_loop->registerWrite(
                m_socket->get(),
                std::bind(&IpLookup::connect, this, _1),
                m_timeout
            );
            break;
        default:
            m_loop->removeException(handle);
            break;
    }
}

std::string IpLookup::hostname() const
{
    if (m_connection)
    {
        return m_connection->getCommonName();
    }
    return {};
}

std::string IpLookup::pin() const
{
    if (m_connection)
    {
        return openssl::Base64::encode(
            m_connection->getPeerCertificatePublicKeyHash()
        );
    }
    return {};
}

}  // namespace dote
