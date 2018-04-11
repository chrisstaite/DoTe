
#pragma once

#include <vector>

struct sockaddr_storage;

namespace dote {

/// \brief  An interface for getting a forwarder for a client
class IForwarders
{
  public:
    virtual ~IForwarders() = default;

    /// \brief  Handle an incoming request
    ///
    /// \param handle   The handle to send the response on
    /// \param client   The client to respond to
    /// \param request  The request to forward on
    virtual void handleRequest(int handle,
                               const sockaddr_storage client,
                               std::vector<char> request) = 0;
};

}  // namespace dote
