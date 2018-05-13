
#pragma once

#include <memory>

namespace dote {

namespace openssl {
class ISslConnection;
}  // namespace openssl

class Loop;
class Socket;
class ConfigParser;

/// \brief  A class that connects to an IP address and gets
///         its certificate hostname and pin
class IpLookup
{
  public:
    /// \brief  Performs a blocking connection to the IP
    ///         given and populates the hostname and pin
    ///
    /// \param config  The configuration of the IP to connect to
    IpLookup(const ConfigParser& config);

    IpLookup(const IpLookup&) = delete;
    IpLookup& operator=(const IpLookup&) = delete;

    /// \brief  Clean up the resources
    ~IpLookup();

    /// \brief  Get the hostname for the connection
    ///
    /// \return  The hostname or empty if an error occurred
    std::string hostname() const;

    /// \brief  Get the certificate pin for the connection
    ///
    /// \return  The certificate pin or empty if an error occurred
    std::string pin() const;

  private:
    /// \brief  Handle the socket connection
    ///
    /// \param handle  The socket handle
    void connect(int handle);

    /// The loop used tp connect
    std::shared_ptr<Loop> m_loop;
    /// The socket that the connection if performed on
    std::shared_ptr<Socket> m_socket;
    /// The connection that was made to the IP address
    std::shared_ptr<openssl::ISslConnection> m_connection;
};

}  // namespace dote
