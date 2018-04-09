
#include "openssl/context.h"

#include <gtest/gtest.h>

namespace dote {
namespace openssl {

class TestingContext : public Context
{
  public:
    TestingContext(const std::string& ciphers) :
        Context(ciphers)
    { }

    using Context::get;
};

TEST(TestContext, ContextCreated)
{
    TestingContext context("ALL");
    EXPECT_NE(nullptr, context.get());
}

}  // namespace openssl
}  // namespace dote
