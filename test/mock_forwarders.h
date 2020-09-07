
#pragma once

#include "i_forwarders.h"

#include <gmock/gmock.h>

namespace dote {

class MockForwarders : public IForwarders
{
  public:
    ~MockForwarders() noexcept
    { }

    MOCK_METHOD5(handleRequest, void(std::shared_ptr<Socket> socket,
                                     const sockaddr_storage& client,
                                     const sockaddr_storage& server,
                                     int interface,
                                     std::vector<char> request));
};

}  // namespace dote
