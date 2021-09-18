
#include "openssl/certificate_utilities.h"

namespace dote {
namespace openssl {

namespace {

#if OPENSSL_VERSION_NUMBER >= 0x010100000
X509* certFromStore(X509_STORE_CTX* store)
{
    return (store == nullptr) ? nullptr : X509_STORE_CTX_get0_cert(store);
}
#else
X509* certFromStore(X509_STORE_CTX* store)
{
    return (store == nullptr) ? nullptr : store->cert;
}
#endif

std::string toString(ASN1_STRING* string)
{
    if (string)
    {
        return std::string(
            reinterpret_cast<const char*>(
#if OPENSSL_VERSION_NUMBER >= 0x010100000
                ASN1_STRING_get0_data(string)
#else
                ASN1_STRING_data(string)
#endif
            ),
            ASN1_STRING_length(string)
        );
    }
    return {};
}

}  // anon namespace

CertificateUtilities::CertificateUtilities(X509* certificate) :
    m_certificate(certificate)
{ }

CertificateUtilities::CertificateUtilities(X509_STORE_CTX* store) :
    m_certificate(certFromStore(store))
{ }

std::vector<unsigned char> CertificateUtilities::getCertificateHash(HashFunction function)
{
    std::vector<unsigned char> hash;
    if (m_certificate)
    {
        const EVP_MD* sha256 = EVP_sha256();
        if (sha256)
        {
            hash.resize(EVP_MAX_MD_SIZE);
            unsigned int length = hash.size();
            if (function(m_certificate, sha256, hash.data(), &length) == 1)
            {
                hash.resize(length);
            }
            else
            {
                hash.clear();
            }
        }
    }
    return hash;
}

std::vector<unsigned char> CertificateUtilities::getHash()
{
    return getCertificateHash(&X509_digest);
}

std::vector<unsigned char> CertificateUtilities::getPublicKeyHash()
{
    return getCertificateHash(&X509_pubkey_digest);
}

std::string CertificateUtilities::getCommonName()
{
    std::string result;
    if (m_certificate)
    {
        int index = X509_NAME_get_index_by_NID(
            X509_get_subject_name(m_certificate),
            NID_commonName,
            -1
        );
        if (index >= 0)
        {
            X509_NAME_ENTRY* commonName = X509_NAME_get_entry(
                X509_get_subject_name(m_certificate), index
            );
            if (commonName)
            {
                result = toString(X509_NAME_ENTRY_get_data(commonName));
            }
        }
    }
    return result;
}

X509* CertificateUtilities::certificate()
{
    return m_certificate;
}

}  // namespace openssl
}  // namespace dote
