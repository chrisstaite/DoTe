
#pragma once

#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>

namespace dote {

sockaddr_storage parse4(const std::string& host, unsigned short port);

sockaddr_storage parse6(const std::string& host, unsigned short port);

}  // namespace dote
