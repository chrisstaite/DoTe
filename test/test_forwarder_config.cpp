
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
    config.addForwarder("ip", "host", "pin");
    EXPECT_NE(config.get(), config.end());
}

TEST(TestForwarderConfig, GetFirst)
{
    ForwarderConfig config;
    config.addForwarder("ip", "host", "pin", 1);
    config.addForwarder("ip2", "host2", "pin2", 2);
    auto first = config.get();
    ASSERT_NE(first, config.end());
    EXPECT_EQ(first->ip, "ip");
    EXPECT_EQ(first->host, "host");
    EXPECT_EQ(first->pin, "pin");
    EXPECT_EQ(first->port, 1);
}

TEST(TestForwarderConfig, DefaultPort)
{
    ForwarderConfig config;
    config.addForwarder("ip", "host", "pin");
    auto first = config.get();
    ASSERT_NE(first, config.end());
    EXPECT_EQ(first->port, 853);
}

}  // namespace dote
