
#include "dote.h"
#include "socket.h"

#include <unistd.h>
#include <sys/socket.h>

namespace dote {

using namespace std::placeholders;

bool Dote::addServer(const char *ip, unsigned short port)
{
    auto serverSocket = Socket::bind(ip, port, Type::UDP);
    if (!serverSocket)
    {
        return false;
    }
    m_serverSockets.emplace_back(std::move(serverSocket));
    m_loop.registerRead(
        m_serverSockets.back()->get(),
        std::bind(&Dote::handleDnsRequest, this, _1, _2)
    );
    return true;
}

void Dote::addForwarder(const char *ip,
                        const char *host,
                        const char *pin,
                        unsigned short port)
{
    Forwarder forwarder{
        std::string(ip), std::string(host), std::string(pin), port
    };
    m_forwarders.emplace_back(std::move(forwarder));
}

void Dote::handleDnsRequest(dote::Loop &loop, int handle)
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

    printf("Got a DNS request of length %zd\n", count);
}

void Dote::run()
{
    m_loop.run();
}

}  // namespace dote
