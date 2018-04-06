
#include "openssl/hostname_verifier.h"

#include <openssl/x509.h>
#include <gtest/gtest.h>

namespace dote {
namespace openssl {

class TestHostnameVerifier : public ::testing::Test
{
  public:
    TestHostnameVerifier();
    
    ~TestHostnameVerifier() noexcept;

  protected:
    X509* m_certificate;
    
    void addSanHostname(const std::string& hostname);
    
    void setCommonName(const std::string& hostname);

  private:
    void createCertificate();
};

TestHostnameVerifier::TestHostnameVerifier()
{
    createCertificate();
}

TestHostnameVerifier::~TestHostnameVerifier() noexcept
{
    if (m_certificate)
    {
        X509_free(m_certificate);
    }
}

void TestHostnameVerifier::createCertificate()
{
    m_certificate = X509_new();
    ASSERT_NE(nullptr, m_certificate);
}

void TestHostnameVerifier::addSanHostname(const std::string &hostname)
{

}

void TestHostnameVerifier::setCommonName(const std::string &hostname)
{

}

}  // namespace openssl
}  // namespace dote
