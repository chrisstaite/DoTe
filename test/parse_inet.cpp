
#include "parse_inet.h"

#include <cstring>

namespace dote {

sockaddr_storage parse4(const std::string& host, unsigned short port)
{
    sockaddr_storage storage;
    storage.ss_family = AF_INET;
    auto& ip4 = reinterpret_cast<sockaddr_in&>(storage);
    inet_pton(AF_INET, host.c_str(), &ip4.sin_addr);
    memset(ip4.sin_zero, 0, sizeof(ip4.sin_zero));
    ip4.sin_port = htons(port);
    return storage;
}

sockaddr_storage parse6(const std::string& host, unsigned short port)
{
    sockaddr_storage storage;
    storage.ss_family = AF_INET6;
    auto& ip6 = reinterpret_cast<sockaddr_in6&>(storage);
    inet_pton(AF_INET6, host.c_str(), &ip6.sin6_addr);
    ip6.sin6_port = htons(port);
    return storage;
}

}  // namespace dote
