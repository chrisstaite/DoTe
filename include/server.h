
#pragma once

#include "config_parser.h"
#include "i_loop.h"

#include <vector>
#include <memory>

namespace dote {

class Socket;
class IForwarders;

/// \brief  The UDP server to recieve connections on
class Server
{
  public:
    /// \brief  Create a DoTe client/server
    ///
    /// \param loop        The looper to use to read
    /// \param forwarders  The forwarder storage
    Server(std::shared_ptr<ILoop> loop,
           std::shared_ptr<IForwarders> forwarders);

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /// \brief  Remove the server sockets from the loop
    ~Server();

    /// \brief  Add a server interface
    ///
    /// \param config  The configuration to add
    ///
    /// \return  True if able to add the server
    bool addServer(const ConfigParser::Server& config);

  private:
    /// \brief  Handle an incoming packet on the server
    ///
    /// \param handle  The handle that the read event is on
    void handleDnsRequest(int handle);

    /// The looper to read using
    std::shared_ptr<ILoop> m_loop;
    /// The available forwarders
    std::shared_ptr<IForwarders> m_forwarders;
    using SocketAndRegistration = std::pair<std::shared_ptr<Socket>, ILoop::Registration>;
    /// The sockets that we are recieving from and their read registrations.
    std::vector<SocketAndRegistration> m_serverSockets;
};

}  // namespace dote
