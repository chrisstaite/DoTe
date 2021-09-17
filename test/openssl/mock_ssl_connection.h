
#pragma once

#include "openssl/i_ssl_connection.h"

#include <gmock/gmock.h>

namespace dote {
namespace openssl {

class MockSslConnection : public ISslConnection
{
  public:
    ~MockSslConnection() noexcept
    { }

    MOCK_METHOD1(setSocket, void(int));
    MOCK_METHOD0(disableVerification, void());
    MOCK_METHOD1(setVerifier, void(Verifier));
    MOCK_METHOD0(getPeerCertificatePublicKeyHash, std::vector<unsigned char>());
    MOCK_METHOD0(getCommonName, std::string());
    MOCK_METHOD0(connect, Result());
    MOCK_METHOD0(shutdown, Result());
    MOCK_METHOD1(write, Result(const std::vector<char>&));
    MOCK_METHOD1(read, Result(std::vector<char>&));
};

}  // namespace openssl
}  // namespace dote
