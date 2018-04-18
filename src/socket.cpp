
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

/// \brief  Convert an AF_* to a PF_*
///
/// \param family  The family to convert to a domain
///
/// \return  The domain for the given family or -1
int toDomain(int family)
{
    switch (family)
    {
        case AF_INET:
            return PF_INET;
        case AF_INET6:
            return PF_INET6;
        default:
            return -1;
    }
}

/// \brief  Get the length of sockaddr from a family
///
/// \param family  The family to get the sockaddr length
///
/// \return  The length of the sockaddr for the family or
///          zero if unknown
int addressLength(int family)
{
    switch (family)
    {
        case AF_INET:
            return sizeof(sockaddr_in);
        case AF_INET6:
            return sizeof(sockaddr_in6);
        default:
            return 0;
    }
}

}  // anon namespace

Socket::Socket(int handle) :
    m_handle(handle)
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

Socket::Socket(int domain, Type type) :
    Socket(socket(domain, toType(type), 0))
{ }

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

std::shared_ptr<Socket> Socket::connect(const sockaddr_storage& address,
                                        Type type)
{
    auto socket = std::make_shared<Socket>(
        toDomain(address.ss_family), type
    );
    if (socket && !socket->connect(
                reinterpret_cast<const sockaddr*>(&address),
                addressLength(address.ss_family)
            ))
    {
        socket.reset();
    }
    return socket;
}

std::shared_ptr<Socket> Socket::bind(const sockaddr_storage& address,
                                     Type type)
{
    auto socket = std::make_shared<Socket>(
        toDomain(address.ss_family), type
    );
    if (socket && !socket->bind(
                reinterpret_cast<const sockaddr*>(&address),
                addressLength(address.ss_family)
            ))
    {
        socket.reset();
    }
    return socket;
}

bool Socket::connect(const sockaddr* address, size_t addressLength)
{
    if (m_handle == -1)
    {
        return false;
    }
    if (::connect(m_handle, address, addressLength) != 0)
    {
        return (errno == EINPROGRESS);
    }
    return true;
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
