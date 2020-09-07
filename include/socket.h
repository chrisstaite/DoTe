
#pragma once

#include <memory>

struct sockaddr;
struct sockaddr_storage;

namespace dote {

/// \brief  A wrapper around a non-blocking socket
class Socket
{
  public:
    /// \brief  The valid types for sockets
    enum Type : int
    {
        /// TCP socket
        TCP,
        /// UDP socket
        UDP
    };

    /// \brief  Wrap a raw socket handle
    ///
    /// \param handle  The handle of the socket to wrap and close
    ///                on destruction
    explicit Socket(int handle);

    /// \brief  Construct a new non-blocking socket
    ///
    /// \param domain  The domain (PF_INET or PF_INET6)
    /// \param type    The type of the socket
    Socket(int domain, Type type);

    /// \brief  Construct a new non-blocking socket and connect it to the IP
    ///
    /// \param address  The address to connect to
    /// \param type     The type of socket to create
    ///
    /// \return  The newly created and connected socket or nullptr
    static std::shared_ptr<Socket> connect(const sockaddr_storage& address,
                                           Type type);

    /// \brief  Context a new non-blocking socket and bind it to the IP
    ///
    /// \param address  The bind to connect to
    /// \param type     The type of socket to create
    ///
    /// \return  The newly created and bound socket or nullptr
    static std::shared_ptr<Socket> bind(const sockaddr_storage& address,
                                        Type type);

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    /// \brief  Calls close
    ~Socket();

    /// \brief  Shutdown and close the socket
    void close();

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

    /// \brief  Enable the packet info for this socket
    ///
    /// \return  True if the packet info was set on the socket
    bool enablePacketInfo();

  private:
    /// The socket handle or -1 if it doesn't have one
    int m_handle;
    /// The domain of the socket
    int m_domain;
};

}  // namespace dote
