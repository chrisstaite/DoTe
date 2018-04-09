
#include "openssl/base64.h"

#include <gtest/gtest.h>

namespace dote {
namespace openssl {

TEST(TestBase64, EmptyToEmpty)
{
    EXPECT_EQ(0u, Base64::decode("").size());
}

TEST(TestBase64, InvalidCharToEmpty)
{
    EXPECT_EQ(0u, Base64::decode("*A==").size());
}

TEST(TestBase64, Invalid1ToEmpty)
{
    EXPECT_EQ(0u, Base64::decode("a").size());
}

TEST(TestBase64, Invalid2ToEmpty)
{
    EXPECT_EQ(0u, Base64::decode("aa").size());
}

TEST(TestBase64, Invalid3ToEmpty)
{
    EXPECT_EQ(0u, Base64::decode("aaa").size());
}

TEST(TestBase64, DecodeByte)
{
    std::vector<unsigned char> expected{
        0x01
    };
    EXPECT_EQ(expected, Base64::decode("AQ=="));
}

TEST(TestBase64, PaddingInsteadOfA)
{
    std::vector<unsigned char> expected{
        0x01
    };
    EXPECT_EQ(expected, Base64::decode("=Q=="));
}

TEST(TestBase64, ExtraPaddingLocation)
{
    std::vector<unsigned char> expected{
        0x01
    };
    EXPECT_EQ(expected, Base64::decode("AQ==="));
}

TEST(TestBase64, DecodeTwoBytes)
{
    std::vector<unsigned char> expected{
        0x01, 0x02
    };
    EXPECT_EQ(expected, Base64::decode("AQI="));
}

TEST(TestBase64, DecodeThreeBytes)
{
    std::vector<unsigned char> expected{
        0x01, 0x02, 0x03
    };
    EXPECT_EQ(expected, Base64::decode("AQID"));
}

TEST(TestBase64, DecodeFourBytes)
{
    std::vector<unsigned char> expected{
        0x01, 0x02, 0x03, 0x04
    };
    EXPECT_EQ(expected, Base64::decode("AQIDBA=="));
}

}  // namespace openssl
}  // namespace dote
