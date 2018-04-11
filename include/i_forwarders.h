
#pragma once

#include <vector>
#include <memory>

struct sockaddr_storage;

namespace dote {

class Socket;

/// \brief  An interface for getting a forwarder for a client
class IForwarders
{
  public:
    virtual ~IForwarders() = default;

    /// \brief  Handle an incoming request
    ///
    /// \param socket   The socket to send the response on
    /// \param client   The client to respond to
    /// \param request  The request to forward on
    virtual void handleRequest(std::shared_ptr<Socket> socket,
                               const sockaddr_storage client,
                               std::vector<char> request) = 0;
};

}  // namespace dote
