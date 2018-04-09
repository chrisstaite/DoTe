
#include "forwarder_config.h"

#include <gtest/gtest.h>

namespace dote {

TEST(TestForwarderConfig, Empty)
{
    ForwarderConfig config;
    EXPECT_EQ(config.get(), config.end());
}

TEST(TestForwarderConfig, NotEmpty)
{
    ForwarderConfig config;
    EXPECT_TRUE(config.addForwarder("ip", "host", "AQ=="));
    EXPECT_NE(config.get(), config.end());
}

TEST(TestForwarderConfig, GetFirst)
{
    ForwarderConfig config;
    EXPECT_TRUE(config.addForwarder("ip", "host", "AQ==", 1));
    EXPECT_TRUE(config.addForwarder("ip2", "host2", "AQD=", 2));
    auto first = config.get();
    ASSERT_NE(first, config.end());
    EXPECT_EQ(first->ip, "ip");
    EXPECT_EQ(first->host, "host");
    EXPECT_EQ(first->pin, std::vector<unsigned char>{0x1});
    EXPECT_EQ(first->port, 1);
}

TEST(TestForwarderConfig, DefaultPort)
{
    ForwarderConfig config;
    EXPECT_TRUE(config.addForwarder("ip", "host", "AQ=="));
    auto first = config.get();
    ASSERT_NE(first, config.end());
    EXPECT_EQ(first->port, 853);
}

TEST(TestForwarderConfig, InvalidPin)
{
    ForwarderConfig config;
    EXPECT_FALSE(config.addForwarder("ip", "host", "B"));
    EXPECT_EQ(config.get(), config.end());
}

TEST(TestForwarderConfig, EmptyPin)
{
    ForwarderConfig config;
    EXPECT_TRUE(config.addForwarder("ip", "host", "", 1));
    auto first = config.get();
    ASSERT_NE(first, config.end());
    EXPECT_EQ(first->ip, "ip");
    EXPECT_EQ(first->host, "host");
    EXPECT_EQ(first->pin, std::vector<unsigned char>{});
    EXPECT_EQ(first->port, 1);
}

TEST(TestForwarderConfig, EmptyHost)
{
    ForwarderConfig config;
    EXPECT_TRUE(config.addForwarder("ip", "", "AQ==", 1));
    auto first = config.get();
    ASSERT_NE(first, config.end());
    EXPECT_EQ(first->ip, "ip");
    EXPECT_EQ(first->host, "");
    EXPECT_EQ(first->pin, std::vector<unsigned char>{0x01});
    EXPECT_EQ(first->port, 1);
}

}  // namespace dote
