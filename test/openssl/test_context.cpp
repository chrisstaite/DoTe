
#include "openssl/context.h"

#include <gtest/gtest.h>

namespace dote {
namespace openssl {

class TestingContext : public Context
{
  public:
    using Context::get;
};

TEST(TestContext, ContextCreated)
{
    TestingContext context;
    EXPECT_NE(nullptr, context.get());
}

}  // namespace openssl
}  // namespace dote
