
#pragma once

#include <vector>
#include <memory>

namespace dote {

class Loop;
class Socket;
class IForwarders;

/// \brief  The core for DoTe
class Dote
{
  public:
    /// \brief  Create a DoTe client/server
    ///
    /// \param loop        The looper to use to read
    /// \param forwarders  The forwarder storage
    Dote(std::shared_ptr<Loop> loop,
         std::shared_ptr<IForwarders> forwarders);

    Dote(const Dote&) = delete;
    Dote& operator=(const Dote&) = delete;

    /// \brief  Add a server interface
    ///
    /// \param ip    The IP to bind the server to
    /// \param port  The port to bind the server to
    ///
    /// \return  True if able to add the server
    bool addServer(const char* ip, unsigned short port = 53);

  private:
    /// \brief  Handle an incoming packet on the server
    ///
    /// \param handle  The handle that the read event is on
    void handleDnsRequest(int handle);
  
    /// The looper to read using
    std::shared_ptr<Loop> m_loop;
    /// The available forwarders
    std::shared_ptr<IForwarders> m_forwarders;
    /// The sockets that we are recieving from
    std::vector<std::shared_ptr<Socket>> m_serverSockets;
};

}  // namespace dote
