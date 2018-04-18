
#include "openssl/ssl_factory.h"
#include "openssl/context.h"

#include <gtest/gtest.h>

namespace dote {
namespace openssl {

TEST(TestSslFactory, CreateSslConnection)
{
    auto context = std::make_shared<Context>("ALL");
    SslFactory factory(context);
    EXPECT_NE(nullptr, factory.create());
}

TEST(TestSslFactory, CreateSslConnectionNull)
{
    std::shared_ptr<Context> context(nullptr);
    SslFactory factory(context);
    EXPECT_EQ(nullptr, factory.create());
}

}  // namespace openssl
}  // namespace dote
