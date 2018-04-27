
#include "verify_cache.h"

#include <openssl/x509.h>
#include <gmock/gmock.h>

namespace dote {

using namespace std::placeholders;
using ::testing::Return;

class MockVerifier
{
  public:
    MOCK_METHOD1(verify, int(X509_STORE_CTX*));
};

class TestVerifyCache : public ::testing::Test
{
  public:
    TestVerifyCache();
    
    ~TestVerifyCache();

    std::unique_ptr<X509, decltype(X509_free)*> createCertificate();

  protected:
    X509_STORE_CTX* m_context;
    MockVerifier m_mockVerifier;
    openssl::Context::Verifier m_verifier;
};

TestVerifyCache::TestVerifyCache() :
    m_context(X509_STORE_CTX_new()),
    m_mockVerifier(),
    m_verifier(std::bind(&MockVerifier::verify, &m_mockVerifier, _1))
{ }

TestVerifyCache::~TestVerifyCache()
{
    if (m_context)
    {
        X509_STORE_CTX_free(m_context);
    }
}

std::unique_ptr<X509, decltype(X509_free)*> TestVerifyCache::createCertificate()
{
    std::unique_ptr<X509, decltype(X509_free)*> certificate(
        X509_new(), &X509_free
    );
    return certificate;
}

TEST_F(TestVerifyCache, InvalidForwardReturnsZero)
{
    VerifyCache cache(nullptr, 1);
    EXPECT_EQ(0, cache.verify(m_context));
}

TEST_F(TestVerifyCache, TestFailCallsAgain)
{
    VerifyCache cache(m_verifier, 1);
    EXPECT_CALL(m_mockVerifier, verify(m_context))
        .Times(2)
        .WillRepeatedly(Return(0));
    EXPECT_EQ(0, cache.verify(m_context));
    EXPECT_EQ(0, cache.verify(m_context));
}

TEST_F(TestVerifyCache, TestPassDoesNotCallAgain)
{
    VerifyCache cache(m_verifier, 1);
    EXPECT_CALL(m_mockVerifier, verify(m_context))
        .WillOnce(Return(1));
    auto certificate(createCertificate());
    m_context->cert = certificate.get();
    EXPECT_EQ(1, cache.verify(m_context));
    EXPECT_EQ(1, cache.verify(m_context));
}

TEST_F(TestVerifyCache, TestPassCallsAgainOnExpiry)
{
    VerifyCache cache(m_verifier, 0);
    EXPECT_CALL(m_mockVerifier, verify(m_context))
        .Times(2)
        .WillRepeatedly(Return(1));
    auto certificate(createCertificate());
    m_context->cert = certificate.get();
    EXPECT_EQ(1, cache.verify(m_context));
    EXPECT_EQ(1, cache.verify(m_context));
}

}  // namespace dote
