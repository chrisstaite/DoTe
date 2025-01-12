
#include "verify_cache.h"

#include <openssl/x509.h>
#include <openssl/ec.h>
#include <gmock/gmock.h>

#include <memory>

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
    // OpenSSLv3 requires the certificate to be valid to get the hash.
    #if OPENSSL_VERSION_MAJOR >= 3
    std::unique_ptr<EVP_PKEY, decltype(EVP_PKEY_free)*> pkey(
        EVP_EC_gen("secp521r1"), &EVP_PKEY_free
    );
    ASN1_INTEGER_set(X509_get_serialNumber(certificate.get()), 1);
    X509_set_pubkey(certificate.get(), pkey.get());
    X509_sign(certificate.get(), pkey.get(), EVP_sha256());
    #endif
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
    X509_STORE_CTX_set_cert(m_context, certificate.get());
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
    X509_STORE_CTX_set_cert(m_context, certificate.get());
    EXPECT_EQ(1, cache.verify(m_context));
    EXPECT_EQ(1, cache.verify(m_context));
}

}  // namespace dote
