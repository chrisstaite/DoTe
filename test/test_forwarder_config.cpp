
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
    ConfigParser::Forwarder forwarder{{}, "host", {}};
    config.addForwarder(forwarder);
    EXPECT_NE(config.get(), config.end());
}

TEST(TestForwarderConfig, GetFirst)
{
    ForwarderConfig config;
    ConfigParser::Forwarder forwarder{{}, "host", {0x1}};
    config.addForwarder(forwarder);
    ConfigParser::Forwarder forwarder2{{}, "host2", {0x2}};
    config.addForwarder(forwarder2);
    auto first = config.get();
    ASSERT_NE(first, config.end());
    EXPECT_EQ(first->host, "host");
    EXPECT_EQ(first->pin, std::vector<unsigned char>{0x1});
}

}  // namespace dote
