
#pragma once

#include "loop.h"

#include <vector>
#include <string>

namespace dote {

class Socket;

/// \brief  The core for DoTe
class Dote
{
  public:
    /// \brief  Create a DoTe client/server
    Dote() = default;

    Dote(const Dote&) = delete;
    Dote& operator=(const Dote&) = delete;

    /// \brief  Add a server interface
    ///
    /// \param ip    The IP to bind the server to
    /// \param port  The port to bind the server to
    ///
    /// \return  True if able to add the server
    bool addServer(const char* ip, unsigned short port = 53);

    /// \brief  Add a DNS server to forward to
    ///
    /// \param ip    The IP address to connect to and forward
    ///              connections to
    /// \param host  The expected host for the TLS connection
    /// \param pin   The base64 encoded SHA-256 hash of the
    ///              certificate that is connected to
    /// \param port  The port to connect to
    void addForwarder(const char* ip,
                      const char* host,
                      const char* pin,
                      unsigned short port = 853);

    /// \brief  Run the contained loop
    void run();

  private:
    /// \brief  All the details required to connect to a forwarder
    struct Forwarder
    {
        /// The IP to connect to and forward to
        std::string ip;
        /// The host to verify the certificate common name against
        std::string host;
        /// The base64 encoded SHA-256 hash of the certificate
        std::string pin;
        /// The port to connect to
        unsigned short port;
    };
    
    /// \brief  Handle an incoming packet on the server
    ///
    /// \param loop    The looper that is handling this
    /// \param handle  The handle that the read event is on
    void handleDnsRequest(Loop& loop, int handle);
  
    /// The wrapped looper to provide the event loop
    Loop m_loop;
    /// The sockets that we are recieving from
    std::vector<std::shared_ptr<Socket>> m_serverSockets;
    /// Currently open ports for sending requests to
    std::vector<std::shared_ptr<Socket>> m_forwarderSockets;
    /// The available forwarders that can be opened in m_forwarderSockets
    std::vector<Forwarder> m_forwarders;
};

}  // namespace dote
