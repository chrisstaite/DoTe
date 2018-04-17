
#include "server.h"
#include "mock_forwarders.h"
#include "mock_loop.h"
#include "parse_inet.h"

#include <memory>

namespace dote {

using ::testing::_;
using ::testing::SaveArg;

class TestServer : public ::testing::Test
{
  public:
    TestServer() :
        m_loop(std::make_shared<MockLoop>()),
        m_forwarders(std::make_shared<MockForwarders>()),
        m_server(m_loop, m_forwarders)
    { }

  protected:
    void configureCallback(int& handle, ILoop::Callback& callback)
    {
        ConfigParser::Server config{
            parse4("127.0.0.1", 2000)
        };
        EXPECT_CALL(*m_loop, registerRead(_, _))
            .WillOnce(DoAll(SaveArg<0>(&handle), SaveArg<1>(&callback)));
        ASSERT_TRUE(m_server.addServer(config));
        ASSERT_TRUE(callback);
        EXPECT_CALL(*m_loop, removeRead(handle))
            .Times(1);
    }
  
    std::shared_ptr<MockLoop> m_loop;
    std::shared_ptr<MockForwarders> m_forwarders;
    Server m_server;
};

TEST_F(TestServer, NoServerDestroy)
{ }

TEST_F(TestServer, AddServer)
{
    ConfigParser::Server config{
        parse4("127.0.0.1", 2000)
    };
    int handle;
    EXPECT_CALL(*m_loop, registerRead(_, _))
        .WillOnce(SaveArg<0>(&handle));
    EXPECT_TRUE(m_server.addServer(config));
    EXPECT_CALL(*m_loop, removeRead(handle))
        .Times(1);
}

TEST_F(TestServer, AddServerTwice)
{
    ConfigParser::Server config{
        parse4("127.0.0.1", 2000)
    };
    int handle;
    EXPECT_CALL(*m_loop, registerRead(_, _))
        .WillOnce(SaveArg<0>(&handle));
    EXPECT_TRUE(m_server.addServer(config));
    EXPECT_CALL(*m_loop, removeRead(handle))
        .Times(1);
    EXPECT_FALSE(m_server.addServer(config));
}

TEST_F(TestServer, ReadFromUnknown)
{
    ILoop::Callback callback;
    int handle;
    configureCallback(handle, callback);
    callback(handle - 1);
}

}  // namespace dote
