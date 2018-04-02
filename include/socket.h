
#pragma once

#include <memory>

struct sockaddr;

namespace dote {

/// \brief  The valid types for sockets
enum Type : int
{
    /// TCP socket
    TCP,
    /// UDP socket
    UDP
};

/// \brief  A wrapper around a non-blocking socket
class Socket
{
  public:
    /// \brief  Construct a new non-blocking socket
    ///
    /// \param domain  The domain (PF_INET or PF_INET6)
    /// \param type    The type of the socket
    Socket(int domain, Type type);

    /// \brief  Construct a new non-blocking socket and connect it to the IP
    ///
    /// \param ip    The IP address to connect to
    /// \param port  The port to connect to
    /// \param type  The type of socket to create
    static std::shared_ptr<Socket> connect(const char* ip,
                                           unsigned short port,
                                           Type type);

    /// \brief  Context a new non-blocking socket and bind it to the IP
    ///
    /// \param ip    The IP address to bind to
    /// \param port  The port to bind to
    /// \param type  The type of socket to create
    static std::shared_ptr<Socket> bind(const char* ip,
                                        unsigned short port,
                                        Type type);

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    /// \brief  Shutdown and close the socket
    ~Socket();

    /// \brief  Connect the socket to a given address
    ///
    /// \param address        The address structure to connect to
    /// \param addressLength  The length of the given address structure
    ///
    /// \return  True if the socket was connected, false if not
    bool connect(const sockaddr* address, size_t addressLength);

    /// \brief  Bind the socket to a given address
    ///
    /// \param address        The address structure to bind to
    /// \param addressLength  The length of the given address structure
    ///
    /// \return  True if the socket was bound, false if not
    bool bind(const sockaddr* address, size_t addressLength);

    /// \brief  Get the underlying raw handle
    ///
    /// \return  The raw handle or -1 if invalid
    int get();

  private:
    /// The socket handle or -1 if it doesn't have one
    int m_handle;
};

}  // namespace dote
