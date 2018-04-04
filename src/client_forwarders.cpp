
#include "client_forwarders.h"
#include "forwarder_connection.h"

#include <sys/socket.h>
#include <functional>

namespace dote {

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

ClientForwarders::~ClientForwarders()
{ }

std::shared_ptr<ForwarderConnection> ClientForwarders::forwarder(
        int handle, sockaddr_storage& client)
{
    auto clientIt = m_clients.begin();
    auto handleIt = m_handles.begin();
    auto connectionIt = m_connections.begin();
    while (clientIt != m_clients.end())
    {
        if (clientIt->ss_len == client.ss_len &&
                memcmp(&clientIt->ss_family, &client.ss_family, client.ss_len) == 0)
        {
            if ((*connectionIt)->closed())
            {
                // Re-open the connection
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
    struct msghdr message {
        &client, client.ss_len, iov, 1, 0, 0
    };
    if (sendmsg(handle, &message, 0) == -1)
    {
        fprintf(stderr, "Error sending response\n");
    }
}

}  // namespace dote
