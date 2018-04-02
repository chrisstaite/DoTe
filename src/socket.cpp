
#include "socket.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace dote {

namespace {

/// \brief  Convert from a type to the underlying socket type
///
/// \param type  The type to convert
///
/// \return  The type of the socket or -1 if invalid
int toType(Type type)
{
    switch (type)
    {
        case TCP:
            return SOCK_STREAM;
        case UDP:
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
        Type type,
        bool (Socket::*init)(const sockaddr*, size_t))
{
    std::shared_ptr<Socket> socket(nullptr);

    // Try doing IPv4 first
    struct sockaddr_in ip4addr;
    if (inet_pton(AF_INET, ip, &ip4addr.sin_addr) == 1)
    {
        ip4addr.sin_family = AF_INET;
        ip4addr.sin_port = htons(port);
        socket = std::make_shared<Socket>(PF_INET, type);
        if (!((*socket).*init)(
                reinterpret_cast<struct sockaddr*>(&ip4addr),
                sizeof(ip4addr)
            ))
        {
            socket.reset();
        }
    }
    // Try with IPv6 on failure
    else
    {
        struct sockaddr_in6 ip6addr;
        if (inet_pton(AF_INET6, ip, &ip6addr.sin6_addr) == 1)
        {
            ip6addr.sin6_family = AF_INET6;
            ip6addr.sin6_port = htons(port);
            socket = std::make_shared<Socket>(PF_INET6, type);
            if (!((*socket).*init)(
                    reinterpret_cast<struct sockaddr*>(&ip6addr),
                    sizeof(ip6addr)
                ))
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
{ }

Socket::~Socket()
{
    if (m_handle != -1)
    {
        (void) shutdown(m_handle, SHUT_RDWR);
        (void) close(m_handle);
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
