
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
    for (const auto& forwarderConfig : config.forwarders())
    {
        m_config->addForwarder(forwarderConfig);
    }
    m_config->setTimeout(config.timeout());
    m_context->setVerifier(std::bind(&VerifyCache::verify, &m_cache, _1));
}

Dote::~Dote()
{ }

bool Dote::listen(const ConfigParser& config)
{
    bool result = true;
    m_server = std::make_shared<Server>(m_loop, m_forwarders);
    for (const auto& serverConfig : config.servers())
    {
        if (!m_server->addServer(serverConfig))
        {
            Log::err << "Unable to bind to server port";
            result = false;
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
        m_loop->run();
    }
}

void Dote::shutdown()
{
    m_server.reset();
}

}  // namespace dote
