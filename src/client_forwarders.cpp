
#include "client_forwarders.h"
#include "forwarder_connection.h"
#include "loop.h"
#include "forwarder_config.h"
#include "log.h"
#include "socket.h"

#include <arpa/inet.h>
#include <functional>

namespace dote {

using namespace std::placeholders;

ClientForwarders::ClientForwarders(std::shared_ptr<Loop> loop,
                                   std::shared_ptr<ForwarderConfig> config,
                                   std::shared_ptr<openssl::Context> context,
                                   std::size_t maxConnections) :
    m_loop(std::move(loop)),
    m_config(std::move(config)),
    m_context(std::move(context)),
    m_maxConnections(maxConnections),
    m_forwarders(),
    m_queue()
{ }

ClientForwarders::~ClientForwarders() noexcept
{ }

void ClientForwarders::handleRequest(std::shared_ptr<Socket> socket,
                                     const sockaddr_storage& client,
                                     std::vector<char> request)
{
    if (m_forwarders.size() < m_maxConnections)
    {
        sendRequest(std::move(socket), client, std::move(request));
    }
    else
    {
        m_queue.emplace_back(QueuedQuery {
            std::move(socket), client, std::move(request)
        });
    }
}

void ClientForwarders::sendRequest(std::shared_ptr<Socket> socket,
                                   const sockaddr_storage& client,
                                   std::vector<char> request)
{
    auto connection = std::make_shared<ForwarderConnection>(
        m_loop, m_config, m_context
    );
    // Send the request
    if (connection->send(request))
    {
        // On data, handle
        sockaddr_storage clientCopy = client;
        connection->setIncomingCallback(
            [this, socket, clientCopy](ForwarderConnection& connection,
                                       std::vector<char> buffer)
            {
                handleIncoming(socket, clientCopy, std::move(buffer));
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
        sendRequest(
            std::move(front.socket),
            front.client,
            std::move(front.request)
        );
        m_queue.pop_front();
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
                                      std::vector<char> buffer)
{
    if (buffer.size() < 2)
    {
        return;
    }
    unsigned short length =
        ntohs(*reinterpret_cast<unsigned short*>(buffer.data()));
    if (buffer.size() < length + 2)
    {
        return;
    }

    struct iovec iov[1] {
        { &buffer.data()[2], length }
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
    struct msghdr message {
        const_cast<sockaddr_storage*>(&client),
        clientLength, iov, 1, 0, 0
    };
    if (sendmsg(socket->get(), &message, 0) == -1)
    {
        Log::warn << "Unable to send response to DNS request";
    }

    return;
}

}  // namespace dote
