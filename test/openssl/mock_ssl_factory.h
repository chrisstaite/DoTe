
#pragma once

#include "openssl/i_ssl_factory.h"

#include <gmock/gmock.h>

namespace dote {
namespace openssl {

class MockSslFactory : public ISslFactory
{
  public:
    ~MockSslFactory() noexcept
    { }

    MOCK_METHOD0(create, std::shared_ptr<ISslConnection>());
};

}  // namespace openssl
}  // namespace dote
