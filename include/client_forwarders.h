
#pragma once

#include "i_forwarders.h"

#include <vector>

namespace dote {

class Loop;
class ForwarderConfig;

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
    ~ClientForwarders();

    /// \brief  Get the forwarder for the given client
    ///
    /// \param handle  The handle to send the response on
    /// \param client  The client to get the forwarder for
    ///
    /// \return  The forwarder for the given client
    std::shared_ptr<ForwarderConnection> forwarder(
            int handle, sockaddr_storage& client) override;

  private:
    /// \brief  Handle an incoming packet on a given connection
    ///
    /// \param connection  The connection that recieved the buffer
    /// \param buffer      The recieved buffer on the connection
    void incoming(ForwarderConnection& connection,
                  std::vector<char> buffer);

    /// \brief  Handle a connection shutting down
    ///
    /// \param connection  The connection that has shutdown
    void shutdown(ForwarderConnection& connection);

    /// \brief  Handle an incoming packet for a given client
    ///
    /// \param handle  The socket to send the response on
    /// \param client  The client that the response is for
    /// \param buffer  The recieved buffer
    void handleIncoming(int handle,
                        sockaddr_storage& client,
                        std::vector<char> buffer);

    /// \brief  Create a new connection
    ///
    /// \return  The newly created forwarder connection
    std::shared_ptr<ForwarderConnection> newConnection();

    /// The looper to use to manage sockets
    std::shared_ptr<Loop> m_loop;
    /// The configuration to use for the forwarders
    std::shared_ptr<ForwarderConfig> m_config;
    /// The OpenSSL context to use
    std::shared_ptr<openssl::Context> m_context;
    /// The clients for each of the connections in m_connections
    std::vector<sockaddr_storage> m_clients;
    /// The sockets to send responses from
    std::vector<int> m_handles;
    /// The current connections
    std::vector<std::shared_ptr<ForwarderConnection>> m_connections;
};

}  // namespace dote
