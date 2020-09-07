
#include "client_forwarders.h"
#include "forwarder_connection.h"
#include "i_loop.h"
#include "i_forwarder_config.h"
#include "log.h"
#include "socket.h"
#include "dns_packet.h"

#ifdef __APPLE__
#define __APPLE_USE_RFC_3542
#endif

#include <arpa/inet.h>
#include <functional>

namespace dote {

namespace {

/// \brief  Add the source address to the outgoing message
///
/// \param message  The outgoing message to add the address to
/// \param server   The address to send the message from
/// \param interface  The interface to send from or -1
void addSourceAddress(struct msghdr& message, const sockaddr_storage& server, int interface)
{
    cmsghdr* controlMsg = nullptr;
    if (server.ss_family == AF_INET6)
    {
#if defined(IPV6_RECVPKTINFO) || defined(IPV6_PKTINFO)
        message.msg_controllen = CMSG_SPACE(sizeof(in6_pktinfo));

        controlMsg = CMSG_FIRSTHDR(&message);
        controlMsg->cmsg_level = IPPROTO_IPV6;
#ifdef IPV6_PKTINFO
        controlMsg->cmsg_type = IPV6_PKTINFO;
#else
        controlMsg->cmsg_type = IPV6_RECVPKTINFO;
#endif
        controlMsg->cmsg_len = CMSG_LEN(sizeof(in6_pktinfo));

        auto packet = reinterpret_cast<in6_pktinfo*>(CMSG_DATA(controlMsg));
        memset(packet, 0, sizeof(*packet));
        packet->ipi6_addr = reinterpret_cast<const sockaddr_in6*>(&server)->sin6_addr;
        packet->ipi6_ifindex = interface;

        message.msg_controllen = controlMsg->cmsg_len;
#endif
    }
    else if (server.ss_family == AF_INET)
    {
#ifdef IP_PKTINFO
        message.msg_controllen = CMSG_SPACE(sizeof(in_pktinfo));

        controlMsg = CMSG_FIRSTHDR(&message);
        controlMsg->cmsg_level = IPPROTO_IP;
        controlMsg->cmsg_type = IP_PKTINFO;
        controlMsg->cmsg_len = CMSG_LEN(sizeof(in_pktinfo));

        auto packet = reinterpret_cast<in_pktinfo*>(CMSG_DATA(controlMsg));
        memset(packet, 0, sizeof(*packet));
        packet->ipi_spec_dst = reinterpret_cast<const sockaddr_in*>(&server)->sin_addr;
        packet->ipi_ifindex = interface;

        message.msg_controllen = controlMsg->cmsg_len;
#endif
#ifdef IP_SENDSRCADDR
        message.msg_controllen = CMSG_SPACE(sizeof(in_addr));

        controlMsg = CMSG_FIRSTHDR(&message);
        controlMsg->cmsg_level = IPPROTO_IP;
        controlMsg->cmsg_type = IP_SENDSRCADDR;
        controlMsg->cmsg_len = CMSG_LEN(sizeof(in_addr));

        auto in = reinterpret_cast<in_addr*>(CMSG_DATA(controlMsg));
        *in = reinterpret_cast<const sockaddr_in*>(&server)->sin_addr;

        message.msg_controllen = controlMsg->cmsg_len;
#endif
    }
}

}  // anon namespace

using namespace std::placeholders;

ClientForwarders::ClientForwarders(std::shared_ptr<ILoop> loop,
                                   std::shared_ptr<IForwarderConfig> config,
                                   std::shared_ptr<openssl::ISslFactory> ssl,
                                   std::size_t maxConnections) :
    m_loop(std::move(loop)),
    m_config(std::move(config)),
    m_ssl(std::move(ssl)),
    m_maxConnections(maxConnections),
    m_forwarders(),
    m_queue()
{ }

ClientForwarders::~ClientForwarders() noexcept
{ }

void ClientForwarders::handleRequest(std::shared_ptr<Socket> socket,
                                     const sockaddr_storage& client,
                                     const sockaddr_storage& server,
                                     int interface,
                                     std::vector<char> request)
{
    if (m_forwarders.size() < m_maxConnections)
    {
        sendRequest(
            std::move(socket), client, server, interface, std::move(request)
        );
    }
    else
    {
        Log::debug << "Queuing request, queue length is " << m_queue.size();
        m_queue.emplace_back(QueuedQuery {
            std::move(socket), client, server, interface, std::move(request)
        });
    }
}

void ClientForwarders::sendRequest(std::shared_ptr<Socket> socket,
                                   const sockaddr_storage& client,
                                   const sockaddr_storage& server,
                                   int interface,
                                   std::vector<char> request)
{
    auto connection = std::make_shared<ForwarderConnection>(
        m_loop, m_config, m_ssl
    );
    // Send the request
    if (connection->send(std::move(request)))
    {
        // On data, handle
        sockaddr_storage clientCopy = client;
        sockaddr_storage serverCopy = server;
        connection->setIncomingCallback(
            [this, socket, clientCopy, serverCopy, interface](
                    ForwarderConnection& connection,
                    std::vector<char> buffer)
            {
                handleIncoming(
                    socket, clientCopy, serverCopy, interface, std::move(buffer)
                );
                // Shutdown after result
                connection.shutdown();
            }
        );
        // On shutdown, remove the client
        connection->setShutdownCallback(
            std::bind(&ClientForwarders::handleShutdown, this, _1)
        );

        // Save off the pointer
        m_forwarders.emplace_back(std::move(connection));
    }
    else
    {
        dequeue();
    }
}

void ClientForwarders::dequeue()
{
    if (!m_queue.empty())
    {
        auto& front = m_queue.front();
        std::shared_ptr<Socket> socket = std::move(front.socket);
        sockaddr_storage client = front.client;
        sockaddr_storage server = front.server;
        int interface = front.interface;
        std::vector<char> request = std::move(front.request);
        m_queue.pop_front();
        sendRequest(
            std::move(socket),
            client,
            server,
            interface,
            std::move(request)
        );
        Log::debug << "Sent request from queue, length now " << m_queue.size();
    }
}

void ClientForwarders::handleShutdown(ForwarderConnection& connection)
{
    for (auto it = m_forwarders.begin(); it != m_forwarders.end(); ++it)
    {
        if (it->get() == &connection)
        {
            m_forwarders.erase(it);
            break;
        }
    }
    dequeue();
}

void ClientForwarders::handleIncoming(const std::shared_ptr<Socket>& socket,
                                      const sockaddr_storage& client,
                                      const sockaddr_storage& server,
                                      int interface,
                                      std::vector<char> buffer)
{
    DnsPacket packet(std::move(buffer));
    if (!packet.valid())
    {
        Log::warn << "Discarding invalid response";
        return;
    }

    (void) packet.removeEdnsPadding();

    struct iovec iov[1] {
        { packet.data(), packet.length() }
    };
    socklen_t clientLength = 0;
    if (client.ss_family == AF_INET)
    {
        clientLength = sizeof(sockaddr_in);
    }
    else if (client.ss_family == AF_INET6)
    {
        clientLength = sizeof(sockaddr_in6);
    }
    char controlBuf[256];
    struct msghdr message {
        const_cast<sockaddr_storage*>(&client),
        clientLength, iov, 1, controlBuf, 0, 0
    };

    addSourceAddress(message, server, interface);

    if (sendmsg(socket->get(), &message, 0) == -1)
    {
        Log::warn << "Unable to send response to DNS request";
    }
}

}  // namespace dote
