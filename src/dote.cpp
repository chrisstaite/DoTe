
#include "dote.h"
#include "log.h"
#include "loop.h"
#include "server.h"
#include "config_parser.h"
#include "client_forwarders.h"
#include "forwarder_config.h"
#include "openssl/context.h"
#include "openssl/ssl_factory.h"

#include <openssl/x509.h>
#include <arpa/inet.h>
#include <netinet/in.h>

namespace dote {

using namespace std::placeholders;

Dote::Dote(const ConfigParser& config) :
    m_loop(std::make_shared<Loop>()),
    m_config(std::make_shared<ForwarderConfig>()),
    m_context(std::make_shared<openssl::Context>(config.ciphers())),
    m_forwarders(std::make_shared<ClientForwarders>(
        m_loop,
        m_config,
        std::make_shared<openssl::SslFactory>(m_context),
        config.maxConnections()
    )),
    m_server(nullptr),
    m_cache(&X509_verify_cert, CACHE_SECONDS)
{
    setForwarders(config);
    m_config->setTimeout(config.timeout());
    m_context->setChainVerifier(std::bind(&VerifyCache::verify, &m_cache, _1));
}

void Dote::setForwarders(const ConfigParser& config)
{
    m_config->clear();
    for (const auto& forwarderConfig : config.forwarders())
    {
        m_config->addForwarder(forwarderConfig);
    }
}

bool Dote::listen(const ConfigParser& config)
{
    bool result = true;
    m_server = std::make_shared<Server>(m_loop, m_forwarders);
    for (const auto& serverConfig : config.servers())
    {
        char ip[64];
        switch(serverConfig.address.ss_family) {
            case AF_INET:
                inet_ntop(AF_INET, &reinterpret_cast<const struct sockaddr_in&>(serverConfig.address).sin_addr, ip, sizeof(ip));
                break;

            case AF_INET6:
                inet_ntop(AF_INET6, &reinterpret_cast<const struct sockaddr_in6&>(serverConfig.address).sin6_addr, ip, sizeof(ip));
                break;
            default:
                ip[0] = '\0';
                break;
        }
        if (!m_server->addServer(serverConfig))
        {
            Log::err << "Unable to bind to server port " << ip;
            result = false;
        }
        else
        {
            Log::info << "Bound server " << ip;
        }
    }
    if (!result)
    {
        m_server.reset();
    }
    return result;
}

void Dote::run()
{
    if (m_loop)
    {
        Log::info << "DoTe started and running";
        m_loop->run();
    }
}

void Dote::shutdown()
{
    m_server.reset();
}

std::shared_ptr<Loop> Dote::looper()
{
    return m_loop;
}

}  // namespace dote
