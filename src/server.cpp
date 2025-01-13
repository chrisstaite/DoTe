
#include "server.h"
#include "socket.h"
#include "i_loop.h"
#include "forwarder_connection.h"
#include "i_forwarders.h"
#include "log.h"

#ifdef __APPLE__
#define __APPLE_USE_RFC_3542
#endif

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace dote {

namespace {

void getDestinationAddress(msghdr& message, sockaddr_storage& dstAddr, int& ifIndex)
{
    // Process ancillary data  received in msgheader - cmsg(3)
    for (auto controlMsg = CMSG_FIRSTHDR(&message);
         controlMsg != nullptr;
         controlMsg = CMSG_NXTHDR(&message, controlMsg))
    {
#ifdef IP_PKTINFO
#ifdef __APPLE__
        constexpr int level = IP_PKTINFO;
#else
        constexpr int level = SOL_IP;
#endif
        if (controlMsg->cmsg_level == level &&
                controlMsg->cmsg_type == IP_PKTINFO)
        {
            auto i = reinterpret_cast<in_pktinfo*>(CMSG_DATA(controlMsg));
            reinterpret_cast<sockaddr_in*>(&dstAddr)->sin_addr = i->ipi_addr;
            dstAddr.ss_family = AF_INET;
            ifIndex = i->ipi_ifindex;
        }
#endif

#ifdef IP_RECVDSTADDR
        if (controlMsg->cmsg_level == IPPROTO_IP &&
                controlMsg->cmsg_type == IP_RECVDSTADDR)
        {
            auto i = reinterpret_cast<in_addr*>(CMSG_DATA(controlMsg));
            reinterpret_cast<sockaddr_in*>(&dstAddr)->sin_addr = *i;
            dstAddr.ss_family = AF_INET;
        }
#endif

#if defined(IPV6_PKTINFO) || defined(IPV6_RECVPKTINFO)
#ifdef IPV6_RECVPKTINFO
        constexpr int type = IPV6_RECVPKTINFO;
#else
        constexpr int type = IPV6_PKTINFO;
#endif
        if (controlMsg->cmsg_level == IPPROTO_IPV6 &&
                controlMsg->cmsg_type == type)
        {
            auto i = reinterpret_cast<in6_pktinfo*>(CMSG_DATA(controlMsg));
            reinterpret_cast<sockaddr_in6*>(&dstAddr)->sin6_addr = i->ipi6_addr;
            dstAddr.ss_family = AF_INET6;
            ifIndex = i->ipi6_ifindex;
        }
#endif
    }
}

}  // anon namespace

using namespace std::placeholders;

Server::Server(std::shared_ptr<ILoop> loop,
               std::shared_ptr<IForwarders> forwarders) :
    m_loop(std::move(loop)),
    m_forwarders(std::move(forwarders))
{ }

Server::~Server() = default;

bool Server::addServer(const ConfigParser::Server& config)
{
    auto serverSocket = Socket::bind(config.address, Socket::Type::UDP);
    if (!serverSocket)
    {
        return false;
    }
    if (!serverSocket->enablePacketInfo())
    {
        Log::warn << "Unable to get recieve address for packets";
    }
    auto registration = m_loop->registerRead(
        serverSocket->get(),
        std::bind(&Server::handleDnsRequest, this, _1),
        0
    );
    m_serverSockets.emplace_back(std::move(serverSocket), std::move(registration));
    return true;
}

void Server::handleDnsRequest(int handle)
{
    // Get the socket for this handle
    std::shared_ptr<Socket> handleSocket;
    for (auto& socket_registration : m_serverSockets)
    {
        if (socket_registration.first->get() == handle)
        {
            handleSocket = socket_registration.first;
            break;
        }
    }
    if (!handleSocket)
    {
        Log::warn << "Request from unknown socket";
        return;
    }

    constexpr size_t SIZE_LENGTH = sizeof(unsigned short);
    constexpr size_t DNS_BUFFER = 512;
    std::vector<char> tcpBuffer(DNS_BUFFER + SIZE_LENGTH);
    sockaddr_storage srcAddr;
    iovec iov[1] = {
        { tcpBuffer.data() + SIZE_LENGTH, tcpBuffer.size() - SIZE_LENGTH }
    };
    char controlBuf[256];
    msghdr message = {
        &srcAddr, sizeof(srcAddr), iov, 1, controlBuf, sizeof(controlBuf), 0
    };

    ssize_t count = recvmsg(handle, &message, 0);
    if (count == -1)
    {
        Log::notice << "No message to receive";
        return;
    }
    else if ((message.msg_flags & MSG_TRUNC))
    {
        Log::notice << "DNS request packet was too big";
        return;
    }

    // Construct a TCP DNS request which is two bytes of length
    // followed by the DNS request packet
    tcpBuffer.resize(count + SIZE_LENGTH);
    *reinterpret_cast<unsigned short*>(tcpBuffer.data()) = htons(count);

    sockaddr_storage dstAddr;
    dstAddr.ss_family = AF_UNSPEC;
    int ifIndex = -1;
    getDestinationAddress(message, dstAddr, ifIndex);

    // Send the request
    m_forwarders->handleRequest(
        std::move(handleSocket), srcAddr, dstAddr, ifIndex, std::move(tcpBuffer)
    );
}

}  // namespace dote
