
#include "client_forwarders.h"
#include "socket.h"
#include "parse_inet.h"
#include "mock_loop.h"
#include "mock_forwarder_config.h"
#include "openssl/mock_ssl_factory.h"
#include "openssl/mock_ssl_connection.h"

namespace dote {

using ::testing::Return;

class TestClientForwarders : public ::testing::Test
{
  public:
    TestClientForwarders() :
        m_loop(std::make_shared<MockLoop>()),
        m_config(std::make_shared<MockForwarderConfig>()),
        m_ssl(std::make_shared<openssl::MockSslFactory>())
    { }

  protected:
    std::shared_ptr<MockLoop> m_loop;
    std::shared_ptr<MockForwarderConfig> m_config;
    std::shared_ptr<openssl::MockSslFactory> m_ssl;
};

TEST_F(TestClientForwarders, HandleRequest)
{
    std::vector<char> request(1);
    int fd[2];
    ASSERT_NE(-1, socketpair(PF_LOCAL, SOCK_DGRAM, 0, fd));
    std::shared_ptr<Socket> socketOne(new Socket(fd[0]));
    std::shared_ptr<Socket> socketTwo(new Socket(fd[1]));
    sockaddr_storage client = parse4("127.0.0.1", htons(60000));
    sockaddr_storage server = { 0, AF_UNSPEC };
    int interface = -1;
    ClientForwarders forwarders(m_loop, m_config, m_ssl, 1u);
    std::shared_ptr<openssl::MockSslConnection> connection(
        std::make_shared<openssl::MockSslConnection>()
    );
    EXPECT_CALL(*m_ssl, create())
        .WillOnce(Return(connection));
    std::vector<ConfigParser::Forwarder> configurations;
    configurations.emplace_back(ConfigParser::Forwarder {
        parse4("127.0.0.1", 4000), "", {}
    });
    EXPECT_CALL(*m_config, get())
        .WillOnce(Return(configurations.begin()));
    EXPECT_CALL(*m_config, end())
        .WillOnce(Return(configurations.end()));
    forwarders.handleRequest(socketOne, client, server, interface, request);
}

}  // namespace dote
