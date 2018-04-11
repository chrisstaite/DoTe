
#pragma once

#include "i_forwarders.h"

#include <vector>
#include <memory>

namespace dote {

class Loop;
class ForwarderConfig;
class ForwarderConnection;

namespace openssl {
class Context;
}  // namespace openssl

/// \brief  A collection of connections to forwarders, one for each
///         of the clients
class ClientForwarders : public IForwarders
{
  public:
    /// \brief  Construct an empty set of forwarders
    ClientForwarders(std::shared_ptr<Loop> loop,
                     std::shared_ptr<ForwarderConfig> config,
                     std::shared_ptr<openssl::Context> context);

    ClientForwarders(const ClientForwarders&) = delete;
    ClientForwarders& operator=(const ClientForwarders&) = delete;

    /// \brief  Destroy the forwarders
    ~ClientForwarders() noexcept;

    /// \brief  Handle an incoming request
    ///
    /// \param handle   The handle to send the response on
    /// \param client   The client to respond to
    /// \param request  The request to forward on
    void handleRequest(int handle,
                       const sockaddr_storage client,
                       std::vector<char> request) override;

  private:
    /// \brief  Handle an incoming packet for a given client
    ///
    /// \param handle  The socket to send the response on
    /// \param client  The client that the response is for
    /// \param buffer  The recieved buffer
    void handleIncoming(int handle,
                        const sockaddr_storage& client,
                        std::vector<char> buffer);

    /// \brief  Handle the shutdown of a client
    ///
    /// \param connection  The connection that has shutdown
    void handleShutdown(ForwarderConnection& connection);

    /// The looper to use to manage sockets
    std::shared_ptr<Loop> m_loop;
    /// The configuration to use for the forwarders
    std::shared_ptr<ForwarderConfig> m_config;
    /// The OpenSSL context to use
    std::shared_ptr<openssl::Context> m_context;
    /// The currently open connections to forwarders
    std::vector<std::shared_ptr<ForwarderConnection>> m_forwarders;
};

}  // namespace dote
