
#include "server.h"
#include "socket.h"
#include "loop.h"
#include "forwarder_connection.h"
#include "i_forwarders.h"
#include "log.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace dote {

using namespace std::placeholders;

Server::Server(std::shared_ptr<Loop> loop,
               std::shared_ptr<IForwarders> forwarders) :
    m_loop(std::move(loop)),
    m_forwarders(std::move(forwarders)),
    m_serverSockets()
{ }

bool Server::addServer(const ConfigParser::Server& config)
{
    auto serverSocket = Socket::bind(config.address, Socket::Type::UDP);
    if (!serverSocket)
    {
        return false;
    }
    m_serverSockets.emplace_back(std::move(serverSocket));
    m_loop->registerRead(
        m_serverSockets.back()->get(),
        std::bind(&Server::handleDnsRequest, this, _1)
    );
    return true;
}

void Server::handleDnsRequest(int handle)
{
    // Get the socket for this handle
    std::shared_ptr<Socket> handleSocket;
    for (auto& socket : m_serverSockets)
    {
        if (socket->get() == handle)
        {
            handleSocket = socket;
            break;
        }
    }
    if (!handleSocket)
    {
        Log::warn << "Request from unknown socket";
        return;
    }

    char buffer[512];
    sockaddr_storage src_addr;
    iovec iov[1] = {
        { buffer, sizeof(buffer) }
    };
    msghdr message = {
        &src_addr, sizeof(src_addr), iov, 1, 0, 0
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
    std::vector<char> tcpBuffer;
    tcpBuffer.resize(count + 2);
    *reinterpret_cast<unsigned short*>(tcpBuffer.data()) = htons(count);
    std::copy(buffer, &buffer[count], tcpBuffer.begin() + 2);

    // Send the request
    m_forwarders->handleRequest(
        std::move(handleSocket), src_addr, std::move(tcpBuffer)
    );
}

}  // namespace dote
