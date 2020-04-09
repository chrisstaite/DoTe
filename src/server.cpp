
#include "server.h"
#include "socket.h"
#include "i_loop.h"
#include "forwarder_connection.h"
#include "i_forwarders.h"
#include "log.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

namespace dote {

using namespace std::placeholders;

Server::Server(std::shared_ptr<ILoop> loop,
               std::shared_ptr<IForwarders> forwarders) :
    m_loop(std::move(loop)),
    m_forwarders(std::move(forwarders)),
    m_serverSockets()
{ }

Server::~Server()
{
    for (auto& socket : m_serverSockets)
    {
        m_loop->removeRead(socket->get());
    }
}

bool Server::addServer(const ConfigParser::Server& config)
{
    auto serverSocket = Socket::bind(config.address, Socket::Type::UDP);
    if (!serverSocket)
    {
        Log::err << "Bind failed: " << strerror(errno);
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

    constexpr size_t SIZE_LENGTH = sizeof(unsigned short);
    constexpr size_t DNS_BUFFER = 512;
    std::vector<char> tcpBuffer(DNS_BUFFER + SIZE_LENGTH);
    sockaddr_storage src_addr;
    iovec iov[1] = {
        { tcpBuffer.data() + SIZE_LENGTH, tcpBuffer.size() - SIZE_LENGTH }
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
    tcpBuffer.resize(count + SIZE_LENGTH);
    *reinterpret_cast<unsigned short*>(tcpBuffer.data()) = htons(count);

    // Send the request
    m_forwarders->handleRequest(
        std::move(handleSocket), src_addr, std::move(tcpBuffer)
    );
}

}  // namespace dote
