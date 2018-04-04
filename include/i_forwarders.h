
#pragma once

#include <memory>

struct sockaddr_storage;

namespace dote {

class ForwarderConnection;

/// \brief  An interface for getting a forwarder for a client
class IForwarders
{
  public:
    virtual ~IForwarders() = default;

    /// \brief  Get the forwarder for the given client
    ///
    /// \param handle  The handle to send the response on
    /// \param client  The client to get the forwarder for
    ///
    /// \return  The forwarder for the given client
    virtual std::shared_ptr<ForwarderConnection> forwarder(
            int handle, sockaddr_storage& client) = 0;
};

}  // namespace dote
