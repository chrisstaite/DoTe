
#include "server.h"
#include "socket.h"
#include "loop.h"
#include "forwarder_connection.h"
#include "i_forwarders.h"

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

bool Server::addServer(const char *ip, unsigned short port)
{
    auto serverSocket = Socket::bind(ip, port, Socket::Type::UDP);
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
        fprintf(stderr, "No message to receive\n");
        return;
    }
    else if ((message.msg_flags & MSG_TRUNC))
    {
        fprintf(stderr, "DNS request packet was too big\n");
        return;
    }

    // Get a forwarder for this client
    auto forwarder = m_forwarders->forwarder(handle, src_addr);

    // Construct a TCP DNS request which is two bytes of length
    // followed by the DNS request packet
    std::vector<char> tcpBuffer;
    tcpBuffer.resize(count + 2);
    *reinterpret_cast<unsigned short*>(tcpBuffer.data()) = htons(count);
    std::copy(buffer, &buffer[count], tcpBuffer.begin() + 2);

    // Send the request
    forwarder->send(std::move(tcpBuffer));
}

}  // namespace dote
