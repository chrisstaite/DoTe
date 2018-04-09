
#include "config_parser.h"

#include <gtest/gtest.h>
#include <unistd.h>

namespace dote {

bool operator==(const ConfigParser::Server& a,
                const ConfigParser::Server& b)
{
    return a.ip == b.ip && a.port == b.port;
}

void PrintTo(const ConfigParser::Server& server, ::std::ostream* os) {
    *os << server.ip << ":" << server.port;
}

bool operator==(const ConfigParser::Forwarder& a,
                const ConfigParser::Forwarder& b)
{
    return a.ip == b.ip && a.port == b.port && a.host == b.host && a.pin == b.pin;
}

void PrintTo(const ConfigParser::Forwarder& server, ::std::ostream* os) {
    *os << server.ip << ":" << server.port;
    if (!server.host.empty())
    {
        *os << " Hostname: " << server.host;
    }
    if (!server.pin.empty())
    {
        *os << " Pin: " << ::testing::PrintToString(server.host);
    }
}

class TestConfigParser : public ::testing::Test
{
  public:
    TestConfigParser()
    {
        optind = 1;
    }

    ~TestConfigParser()
    {
        optind = 1;
    }
};

TEST_F(TestConfigParser, DefaultServers)
{
    const char* const args[] = { "" };
    std::vector<ConfigParser::Server> expected{
        {"127.0.0.1", 53},
        {"::1", 53}
    };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_TRUE(parser.valid());
    EXPECT_EQ(expected, parser.servers());
}

TEST_F(TestConfigParser, OneServerDefaultPort)
{
    const char* const args[] = { "", "-s", "127.0.0.1" };
    std::vector<ConfigParser::Server> expected{
        {"127.0.0.1", 53}
    };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_TRUE(parser.valid());
    EXPECT_EQ(expected, parser.servers());
}

TEST_F(TestConfigParser, OneServer)
{
    const char* const args[] = { "", "-s", "127.0.0.1:5353" };
    std::vector<ConfigParser::Server> expected{
        { "127.0.0.1", 5353 }
    };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_TRUE(parser.valid());
    EXPECT_EQ(expected, parser.servers());
}

TEST_F(TestConfigParser, OneServerBadPort)
{
    const char* const args[] = { "", "-s", "127.0.0.1:h5353" };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_FALSE(parser.valid());
}

TEST_F(TestConfigParser, OneServerInvalidPort)
{
    const char* const args[] = { "", "-s", "127.0.0.1:65536" };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_FALSE(parser.valid());
}

TEST_F(TestConfigParser, OneServerIPv6DefaultPort)
{
    const char* const args[] = { "", "-s", "[::1]" };
    std::vector<ConfigParser::Server> expected{
        {"::1", 53}
    };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_TRUE(parser.valid());
    EXPECT_EQ(expected, parser.servers());
}

TEST_F(TestConfigParser, OneServerIPv6)
{
    const char* const args[] = { "", "-s", "[::1]:5353" };
    std::vector<ConfigParser::Server> expected{
        { "::1", 5353 }
    };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_TRUE(parser.valid());
    EXPECT_EQ(expected, parser.servers());
}

TEST_F(TestConfigParser, OnlyHostname)
{
    const char* const args[] = { "", "-h", "domain.com" };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_FALSE(parser.valid());
}

TEST_F(TestConfigParser, OnlyPin)
{
    const char* const args[] = { "", "-p", "AQ==" };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_FALSE(parser.valid());
}

TEST_F(TestConfigParser, OneForwarderDefaultPort)
{
    const char* const args[] = { "", "-f", "1.1.1.1" };
    std::vector<ConfigParser::Forwarder> expected{
        { "1.1.1.1", "", {}, 853 }
    };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_TRUE(parser.valid());
    EXPECT_EQ(expected, parser.forwarders());
}

TEST_F(TestConfigParser, OneForwarderWithPort)
{
    const char* const args[] = { "", "-f", "1.1.1.1:8853" };
    std::vector<ConfigParser::Forwarder> expected{
        { "1.1.1.1", "", {}, 8853 }
    };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_TRUE(parser.valid());
    EXPECT_EQ(expected, parser.forwarders());
}

TEST_F(TestConfigParser, OneForwarderWithHostname)
{
    const char* const args[] = { "", "-f", "1.1.1.1", "-h", "domain.com" };
    std::vector<ConfigParser::Forwarder> expected{
        { "1.1.1.1", "domain.com", {}, 853 }
    };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_TRUE(parser.valid());
    EXPECT_EQ(expected, parser.forwarders());
}

TEST_F(TestConfigParser, OneForwarderWithPin)
{
    const char* const args[] = { "", "-f", "1.1.1.1", "-p", "AQ==" };
    std::vector<ConfigParser::Forwarder> expected{
        { "1.1.1.1", "", { 0x01 }, 853 }
    };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_TRUE(parser.valid());
    EXPECT_EQ(expected, parser.forwarders());
}

TEST_F(TestConfigParser, OneForwarderWithInvalidPin)
{
    const char* const args[] = { "", "-f", "1.1.1.1", "-p", "*Q==" };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_FALSE(parser.valid());
}

TEST_F(TestConfigParser, OpenSSLCiphers)
{
    const char* const args[] = { "", "-c", "ALL" };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_TRUE(parser.valid());
    EXPECT_EQ("ALL", parser.ciphers());
}

TEST_F(TestConfigParser, UnknownOption)
{
    const char* const args[] = { "", "-x", "a" };
    ConfigParser parser(
        sizeof(args) / sizeof(args[0]), const_cast<char* const*>(args)
    );
    EXPECT_FALSE(parser.valid());
}

}  // namespace dote
