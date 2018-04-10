
#include "config_parser.h"

#include <gtest/gtest.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace dote {

bool operator==(const sockaddr_storage& a,
                const sockaddr_storage& b)
{
    if (a.ss_family != b.ss_family)
    {
        return false;
    }
    if (a.ss_family == AF_INET)
    {
        auto& ina = reinterpret_cast<const sockaddr_in&>(a);
        auto& inb = reinterpret_cast<const sockaddr_in&>(b);
        return ina.sin_addr.s_addr == inb.sin_addr.s_addr &&
            ina.sin_port == inb.sin_port &&
            memcmp(
                    &ina.sin_zero, &inb.sin_zero, sizeof(ina.sin_zero)
                ) == 0;
    }
    if (a.ss_family == AF_INET6)
    {
        auto& ina = reinterpret_cast<const sockaddr_in6&>(a);
        auto& inb = reinterpret_cast<const sockaddr_in6&>(b);
        return memcmp(
                    &ina.sin6_addr, &inb.sin6_addr, sizeof(ina.sin6_addr)
                ) == 0 &&
            ina.sin6_port == inb.sin6_port;
    }
    return memcmp(&a, &b, sizeof(sockaddr_storage)) == 0;
}

void PrintTo(const sockaddr_storage& server, ::std::ostream* os) {
    if (server.ss_family == AF_INET)
    {
        char output[16];
        auto& in4 = reinterpret_cast<const sockaddr_in&>(server);
        *os << inet_ntop(
                in4.sin_family,
                &in4.sin_addr,
                output,
                sizeof(output)
            ) << ":" << ntohs(in4.sin_port);
    }
    else if (server.ss_family == AF_INET6)
    {
        char output[40];
        auto& in6 = reinterpret_cast<const sockaddr_in6&>(server);
        *os << "[" << inet_ntop(
                in6.sin6_family,
                &in6.sin6_addr,
                output,
                sizeof(output)
            ) << "]:" << ntohs(in6.sin6_port);
    }
    else
    {
        *os << "Unknown family";
    }
}

bool operator==(const ConfigParser::Server& a,
                const ConfigParser::Server& b)
{
    return a.address == b.address;
}

void PrintTo(const ConfigParser::Server& server, ::std::ostream* os) {
    PrintTo(server.address, os);
}

bool operator==(const ConfigParser::Forwarder& a,
                const ConfigParser::Forwarder& b)
{
    return a.remote == b.remote && a.host == b.host && a.pin == b.pin;
}

void PrintTo(const ConfigParser::Forwarder& server, ::std::ostream* os) {
    PrintTo(server.remote, os);
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

  protected:
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
};

TEST_F(TestConfigParser, DefaultServers)
{
    const char* const args[] = { "" };
    std::vector<ConfigParser::Server> expected{
        { parse4("127.0.0.1", 53) },
        { parse6("::1", 53) }
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
        { parse4("127.0.0.1", 53) }
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
        {parse4("127.0.0.1", 5353) }
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
        { parse6("::1", 53) }
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
        {parse6("::1", 5353) }
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
        { parse4("1.1.1.1", 853), "", {} }
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
        { parse4("1.1.1.1", 8853), "", {} }
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
        {parse4("1.1.1.1", 853), "domain.com", {} }
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
        { parse4("1.1.1.1", 853), "", { 0x01 } }
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
