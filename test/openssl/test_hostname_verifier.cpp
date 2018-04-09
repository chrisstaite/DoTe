
#include "openssl/hostname_verifier.h"

#include <openssl/x509.h>
#include <openssl/x509v3.h>
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
    int critical;
    auto sanNames = reinterpret_cast<STACK_OF(GENERAL_NAME)*>(
        X509_get_ext_d2i(
            m_certificate, NID_subject_alt_name, &critical, nullptr
        )
    );
    if (sanNames == nullptr)
    {
        // No names yet, create some
        X509_EXTENSION* extension = X509V3_EXT_conf_nid(
            nullptr,
            nullptr,
            NID_subject_alt_name,
            const_cast<char*>(("DNS:" + hostname).c_str())
        );
        ASSERT_NE(nullptr, extension);
        int ret = X509_add_ext(m_certificate, extension, -1);
        X509_EXTENSION_free(extension);
        ASSERT_NE(0, ret);
    }
    else
    {
        // Add a name to an existing set
        GENERAL_NAME *newName = GENERAL_NAME_new();
        ASSERT_NE(nullptr, newName);
        newName->type = GEN_DNS;
        newName->d.dNSName = ASN1_IA5STRING_new();
        if (newName->d.dNSName == nullptr)
        {
            GENERAL_NAME_free(newName);
        }
        ASSERT_NE(nullptr, newName->d.dNSName);
        ASSERT_EQ(1, ASN1_STRING_set(
            newName->d.dNSName, hostname.c_str(), hostname.length()
        ));
        sk_GENERAL_NAME_push(sanNames, newName);

        ASSERT_EQ(1, X509_add1_ext_i2d(
            m_certificate, NID_subject_alt_name, sanNames, critical,
            X509V3_ADD_REPLACE
        ));
    }
}

void TestHostnameVerifier::setCommonName(const std::string &hostname)
{
    X509_NAME* subjectName = X509_NAME_new();
    ASSERT_NE(nullptr, subjectName);
    ASSERT_EQ(1, X509_NAME_add_entry_by_NID(
        subjectName, NID_commonName, MBSTRING_UTF8,
        reinterpret_cast<unsigned char*>(const_cast<char*>(hostname.c_str())),
        hostname.size(), -1, 0
    ));
    X509_set_subject_name(m_certificate, subjectName);
}

TEST_F(TestHostnameVerifier, MatchSanNoName)
{
    addSanHostname("domain.com");
    HostnameVerifier verifier(m_certificate);
    EXPECT_TRUE(verifier.isValid(""));
}

TEST_F(TestHostnameVerifier, MatchCommonNameNoName)
{
    setCommonName("domain.com");
    HostnameVerifier verifier(m_certificate);
    EXPECT_TRUE(verifier.isValid(""));
}

TEST_F(TestHostnameVerifier, MatchSan)
{
    std::string name("domain.com");
    addSanHostname(name);
    HostnameVerifier verifier(m_certificate);
    EXPECT_TRUE(verifier.isValid(name));
}

TEST_F(TestHostnameVerifier, MatchSecondSan)
{
    std::string name("domain.com");
    addSanHostname("example.com");
    addSanHostname(name);
    HostnameVerifier verifier(m_certificate);
    EXPECT_TRUE(verifier.isValid(name));
}

TEST_F(TestHostnameVerifier, MatchCommonName)
{
    std::string name("domain.com");
    setCommonName(name);
    HostnameVerifier verifier(m_certificate);
    EXPECT_TRUE(verifier.isValid(name));
}

TEST_F(TestHostnameVerifier, MatchWildcardCommonName)
{
    setCommonName("*.domain.com");
    HostnameVerifier verifier(m_certificate);
    EXPECT_TRUE(verifier.isValid("www.domain.com"));
}

TEST_F(TestHostnameVerifier, NoMatchWildcardNoSubdomainCommonName)
{
    setCommonName("*.domain.com");
    HostnameVerifier verifier(m_certificate);
    EXPECT_FALSE(verifier.isValid("domain.com"));
}

TEST_F(TestHostnameVerifier, NoMatchWildcardNoneSubdomainCommonName)
{
    setCommonName("*w.domain.com");
    HostnameVerifier verifier(m_certificate);
    EXPECT_FALSE(verifier.isValid("www.domain.com"));
}

TEST_F(TestHostnameVerifier, NoMatchWildcardSingleDotCommonName)
{
    setCommonName("*.com");
    HostnameVerifier verifier(m_certificate);
    EXPECT_FALSE(verifier.isValid("domain.com"));
}

TEST_F(TestHostnameVerifier, MatchWildcardSan)
{
    addSanHostname("*.domain.com");
    HostnameVerifier verifier(m_certificate);
    EXPECT_TRUE(verifier.isValid("www.domain.com"));
}

}  // namespace openssl
}  // namespace dote
