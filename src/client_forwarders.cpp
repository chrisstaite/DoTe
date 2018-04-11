
#include "client_forwarders.h"
#include "forwarder_connection.h"
#include "log.h"

#include <sys/socket.h>
#include <arpa/inet.h>

#include <functional>
#include <cstring>

namespace dote {

namespace {

/// \brief  Compare two client addresses for equality
///
/// \param client1  The first address
/// \param client2  The second address
///
/// \return  True if the addresses are the same
bool isClient(const sockaddr_storage& client1, const sockaddr_storage& client2)
{
    if (client1.ss_family != client2.ss_family)
    {
        return false;
    }
    switch (client1.ss_family)
    {
        case AF_INET:
            return memcmp(&client1, &client2, sizeof(sockaddr_in)) == 0;
        case AF_INET6:
            return memcmp(&client1, &client2, sizeof(sockaddr_in6)) == 0;
        default:
            return memcmp(&client1, &client2, sizeof(sockaddr_storage)) == 0;
    }
}

}  // anon namespace

using namespace std::placeholders;

ClientForwarders::ClientForwarders(std::shared_ptr<Loop> loop,
                                   std::shared_ptr<ForwarderConfig> config,
                                   std::shared_ptr<openssl::Context> context) :
    m_loop(std::move(loop)),
    m_config(std::move(config)),
    m_context(std::move(context)),
    m_clients(),
    m_handles(),
    m_connections()
{ }

ClientForwarders::~ClientForwarders() noexcept
{ }

std::shared_ptr<ForwarderConnection> ClientForwarders::forwarder(
        int handle, sockaddr_storage& client)
{
    auto clientIt = m_clients.begin();
    auto handleIt = m_handles.begin();
    auto connectionIt = m_connections.begin();
    while (clientIt != m_clients.end())
    {
        if (isClient(*clientIt, client))
        {
            if ((*connectionIt)->closed())
            {
                // Re-open the connection
                Log::notice << "Connection was closed when in "
                    << "use, re-opening the connection";
                *connectionIt = newConnection();
            }
            return *connectionIt;
        }
        ++clientIt;
        ++handleIt;
        ++connectionIt;
    }

    // New connection
    m_clients.emplace_back(client);
    m_handles.emplace_back(handle);
    m_connections.emplace_back(newConnection());
    Log::debug << "Opened a new connection to a forwarder";

    return m_connections.back();
}

std::shared_ptr<ForwarderConnection> ClientForwarders::newConnection()
{
    return std::make_shared<ForwarderConnection>(
        m_loop,
        m_config,
        std::bind(&ClientForwarders::incoming, this, _1, _2),
        std::bind(&ClientForwarders::shutdown, this, _1),
        m_context
    );
}

void ClientForwarders::shutdown(ForwarderConnection& connection)
{
    auto clientIt = m_clients.begin();
    auto handleIt = m_handles.begin();
    auto connectionIt = m_connections.begin();
    while (connectionIt != m_connections.end())
    {
        if (connectionIt->get() == &connection)
        {
            // This is the connection that has shutdown
            m_clients.erase(clientIt);
            m_handles.erase(handleIt);
            m_connections.erase(connectionIt);
            break;
        }
        ++clientIt;
        ++handleIt;
        ++connectionIt;
    }
}

void ClientForwarders::incoming(ForwarderConnection& connection,
                                std::vector<char> buffer)
{
    auto clientIt = m_clients.begin();
    auto handleIt = m_handles.begin();
    auto connectionIt = m_connections.begin();
    while (connectionIt != m_connections.end())
    {
        if (connectionIt->get() == &connection)
        {
            // This is the connection to return the result to
            handleIncoming(*handleIt, *clientIt, std::move(buffer));
            if ((*connectionIt)->closed())
            {
                m_clients.erase(clientIt);
                m_handles.erase(handleIt);
                m_connections.erase(connectionIt);
            }
            break;
        }
        ++clientIt;
        ++handleIt;
        ++connectionIt;
    }
}

void ClientForwarders::handleIncoming(int handle,
                                      sockaddr_storage& client,
                                      std::vector<char> buffer)
{
    // TODO: Support caching of partial results
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
        &client, clientLength, iov, 1, 0, 0
    };
    if (sendmsg(handle, &message, 0) == -1)
    {
        Log::warn << "Unable to send response to DNS request";
    }
}

}  // namespace dote
