
#pragma once

#include "verify_cache.h"

#include <memory>

namespace dote {

class Loop;
class Server;
class ConfigParser;
class ForwarderConfig;
class ClientForwarders;

namespace openssl {
class Context;
}  // namespace openssl

/// \brief  A main wrapper around the classes that are required to
///         provide the DoTe server
class Dote
{
  public:
    /// The number of seconds to cache the cerificates for
    static constexpr int CACHE_SECONDS = 30;

    /// \brief  Create a DoTe server from a given config
    ///
    /// \param config  The configuration to use
    Dote(const ConfigParser& config);

    Dote(const Dote&) = delete;
    Dote& operator=(const Dote&) = delete;

    /// \brief  Shut down the server
    ~Dote() = default;
    
    /// \brief  Start listening on the server ports
    ///
    /// \param config  The configuration with the bind ports
    ///
    /// \return  True if all the ports were bound
    bool listen(const ConfigParser& config);

    /// \brief  Run the server
    void run();

    /// \brief  Stop the server
    void shutdown();

  private:
    /// The looper that is used for the server
    std::shared_ptr<Loop> m_loop;
    /// The available forwarders
    std::shared_ptr<ForwarderConfig> m_config;
    /// The OpenSSL context to use
    std::shared_ptr<openssl::Context> m_context;
    /// The current open forwarders
    std::shared_ptr<ClientForwarders> m_forwarders;
    /// The listening servers
    std::shared_ptr<dote::Server> m_server;
    /// The certificate verification cache for m_context
    VerifyCache m_cache;
};

}  // namespace dote
