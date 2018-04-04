
#include "socket.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace dote {

namespace {

/// \brief  Convert from a type to the underlying socket type
///
/// \param type  The type to convert
///
/// \return  The type of the socket or -1 if invalid
int toType(Socket::Type type)
{
    switch (type)
    {
        case Socket::Type::TCP:
            return SOCK_STREAM;
        case Socket::Type::UDP:
            return SOCK_DGRAM;
        default:
            return -1;
    }
}

/// \brief  Create a new socket and then call a function on it,
///         with the correct domain depending on the given IP
///
/// \param ip    The IP address to call the init function for
/// \param port  The port to set the sockaddr to
/// \param type  The type of socket to create
/// \param init  The function to call after
std::shared_ptr<Socket> initialise(
        const char* ip,
        unsigned short port,
        Socket::Type type,
        bool (Socket::*init)(const sockaddr*, size_t))
{
    std::shared_ptr<Socket> socket(nullptr);

    struct sockaddr_in ip4addr;
    struct sockaddr_in6 ip6addr;

    // Try doing IPv4 first
    if (inet_pton(AF_INET, ip, &ip4addr.sin_addr) == 1)
    {
        ip4addr.sin_family = AF_INET;
        ip4addr.sin_port = htons(port);
        memset(ip4addr.sin_zero, 0, sizeof(ip4addr.sin_zero));
        socket = std::make_shared<Socket>(PF_INET, type);
        if (!((*socket).*init)(
                reinterpret_cast<struct sockaddr*>(&ip4addr),
                sizeof(ip4addr)
            ))
        {
            if (errno != EINPROGRESS)
            {
                socket.reset();
            }
        }
    }
    // Try with IPv6 on failure
    else if (inet_pton(AF_INET6, ip, &ip6addr.sin6_addr) == 1)
    {
        ip6addr.sin6_family = AF_INET6;
        ip6addr.sin6_port = htons(port);
        socket = std::make_shared<Socket>(PF_INET6, type);
        if (!((*socket).*init)(
                reinterpret_cast<struct sockaddr*>(&ip6addr),
                sizeof(ip6addr)
            ))
        {
            if (errno != EINPROGRESS)
            {
                socket.reset();
            }
        }
    }

    return socket;
}

}  // anon namespace

Socket::Socket(int domain, Type type) :
    m_handle(socket(domain, toType(type), 0))
{
    // Make the socket non-blocking
    if (m_handle >= 0)
    {
        int flags = fcntl(m_handle, F_GETFL, 0);
        if (flags == -1)
        {
            close();
        }
        else if (fcntl(m_handle, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            close();
        }
    }
}

Socket::~Socket()
{
    close();
}

void Socket::close()
{
    if (m_handle != -1)
    {
        (void) ::shutdown(m_handle, SHUT_RDWR);
        (void) ::close(m_handle);
        m_handle = -1;
    }
}

std::shared_ptr<Socket> Socket::connect(const char *ip,
                                        unsigned short port,
                                        Type type)
{
    return initialise(ip, port, type, &Socket::connect);
}

std::shared_ptr<Socket> Socket::bind(const char *ip,
                                     unsigned short port,
                                     Type type)
{
    return initialise(ip, port, type, &Socket::bind);
}

bool Socket::connect(const sockaddr* address, size_t addressLength)
{
    return m_handle != -1 &&
        ::connect(m_handle, address, addressLength) == 0;
}

bool Socket::bind(const sockaddr* address, size_t addressLength)
{
    return m_handle != -1 &&
        ::bind(m_handle, address, addressLength) == 0;
}

int Socket::get()
{
    return m_handle;
}

}  // namespace dote
