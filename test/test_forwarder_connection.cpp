
#include "forwarder_connection.h"
#include "mock_loop.h"
#include "mock_forwarder_config.h"
#include "openssl/mock_ssl_factory.h"
#include "openssl/mock_ssl_connection.h"

#include <gtest/gtest.h>

namespace dote {

using ::testing::Return;

class TestForwarderConnection : public ::testing::Test
{
  public:
    TestForwarderConnection() :
        m_loop(std::make_shared<MockLoop>()),
        m_config(std::make_shared<MockForwarderConfig>()),
        m_ssl(std::make_shared<openssl::MockSslFactory>()),
        m_connection(std::make_shared<openssl::MockSslConnection>())
    {
        EXPECT_CALL(*m_ssl, create())
            .WillOnce(Return(m_connection));
    }

  protected:
    std::shared_ptr<MockLoop> m_loop;
    std::shared_ptr<MockForwarderConfig> m_config;
    std::shared_ptr<openssl::MockSslFactory> m_ssl;
    std::shared_ptr<openssl::MockSslConnection> m_connection;
};

TEST_F(TestForwarderConnection, CreateNoForwarders)
{
    std::vector<ConfigParser::Forwarder> connections;
    EXPECT_CALL(*m_config, get())
        .WillOnce(Return(connections.begin()));
    EXPECT_CALL(*m_config, end())
        .WillOnce(Return(connections.end()));
    ForwarderConnection connection(m_loop, m_config, m_ssl);
}

}  // namespace dote
