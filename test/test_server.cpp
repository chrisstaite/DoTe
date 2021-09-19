
#include "server.h"
#include "mock_forwarders.h"
#include "mock_loop.h"
#include "parse_inet.h"
#include "socket.h"

#include <memory>

namespace dote {

using ::testing::_;
using ::testing::Invoke;
using ::testing::DoAll;
using ::testing::SaveArg;

class TestServer : public ::testing::Test
{
  public:
    TestServer() :
        m_loop(std::make_shared<MockLoop>()),
        m_forwarders(std::make_shared<MockForwarders>()),
        m_server(m_loop, m_forwarders),
        m_callback(),
        m_handle(-1),
        m_config()
    {
        m_config.address = parse4("127.0.0.1", htons(2000));
        // Check that the address is available
        while (Socket::bind(m_config.address, Socket::UDP) == nullptr)
        {
            auto address = reinterpret_cast<sockaddr_in*>(&m_config.address);
            auto port = ntohs(address->sin_port);
            address->sin_port = htons(port + 1);
        }
    }

  protected:
    void configureCallback()
    {
        EXPECT_CALL(*m_loop, registerRead(_, _, _))
            .WillOnce(Invoke([this](int handle, ILoop::Callback callback, time_t timeout) {
                m_handle = handle;
                m_callback = std::move(callback);
                return true;
            }));
        ASSERT_TRUE(m_server.addServer(m_config));
        ASSERT_TRUE(m_callback);
        EXPECT_CALL(*m_loop, removeRead(m_handle))
            .Times(1);
    }

    std::shared_ptr<MockLoop> m_loop;
    std::shared_ptr<MockForwarders> m_forwarders;
    Server m_server;
    ILoop::Callback m_callback;
    int m_handle;
    ConfigParser::Server m_config;
};

TEST_F(TestServer, NoServerDestroy)
{ }

TEST_F(TestServer, AddServer)
{
    configureCallback();
}

TEST_F(TestServer, AddServerTwice)
{
    configureCallback();
    EXPECT_FALSE(m_server.addServer(m_config));
}

TEST_F(TestServer, ReadFromUnknown)
{
    configureCallback();
    m_callback(m_handle - 1);
}

}  // namespace dote
