
#pragma once

#include "i_forwarders.h"

#include <sys/socket.h>
#include <deque>

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
    /// \param socket   The socket to send the response on
    /// \param client   The client to respond to
    /// \param request  The request to forward on
    void handleRequest(std::shared_ptr<Socket> socket,
                       const sockaddr_storage& client,
                       std::vector<char> request) override;

  private:
    /// The maximum number of outstanding queries at a time
    static constexpr std::size_t MAX_QUERIES = 10u;

    /// \brief  The details of an incoming query that will be
    ///         sent when there's space left
    struct QueuedQuery
    {
        /// The socket to send the reply on
        std::shared_ptr<Socket> socket;
        /// The client to send the reply to
        sockaddr_storage client;
        /// The request to send
        std::vector<char> request;
    };

    /// \brief  Send a request
    ///
    /// \param socket   The socket to send the response on
    /// \param client   The client to respond to
    /// \param request  The request to forward on
    void sendRequest(std::shared_ptr<Socket> socket,
                     const sockaddr_storage& client,
                     std::vector<char> request);

    /// \brief  Handle an incoming packet for a given client
    ///
    /// \param socket  The socket to send the response on
    /// \param client  The client that the response is for
    /// \param buffer  The recieved buffer
    void handleIncoming(const std::shared_ptr<Socket>& socket,
                        const sockaddr_storage& client,
                        std::vector<char> buffer);

    /// \brief  Send a request from the front of the queue
    void dequeue();

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
    /// A queue of requests that will be sent when there's room
    std::deque<QueuedQuery> m_queue;
};

}  // namespace dote
