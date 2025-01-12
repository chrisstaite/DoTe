
#include "openssl/base64.h"

#include <openssl/opensslv.h>
#include <gtest/gtest.h>

namespace dote {
namespace openssl {

TEST(TestBase64, DecodeEmptyToEmpty)
{
    EXPECT_EQ(0u, Base64::decode("").size());
}

TEST(TestBase64, EncodeEmptyToEmpty)
{
    EXPECT_EQ(0u, Base64::encode({}).size());
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

// Behaviour appears to change across platforms now.
TEST(TestBase64, DISABLED_PaddingInsteadOfA)
{
    std::vector<unsigned char> expected{
        0x01
    };
    EXPECT_EQ(expected, Base64::decode("=Q=="));
}

// Behaviour appears to change across platforms now.
TEST(TestBase64, DISABLED_ExtraPaddingLocation)
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

TEST(TestBase64, EncodeSingleByte)
{
    EXPECT_STREQ("AQ==", Base64::encode({ 0x01 }).c_str());
}

TEST(TestBase64, EncodeTwoBytes)
{
    EXPECT_STREQ("AQI=", Base64::encode({ 0x01, 0x02 }).c_str());
}

TEST(TestBase64, EncodeThreeBytes)
{
    EXPECT_STREQ("AQID", Base64::encode({ 0x01, 0x02, 0x03 }).c_str());
}

TEST(TestBase64, EncodeFourBytes)
{
    EXPECT_STREQ("AQIDBA==", Base64::encode({ 0x01, 0x02, 0x03, 0x04 }).c_str());
}

}  // namespace openssl
}  // namespace dote
