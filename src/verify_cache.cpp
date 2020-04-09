
#include "verify_cache.h"

#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

namespace dote {

namespace {

#if OPENSSL_VERSION_NUMBER >= 0x010100000
X509* certFromStore(X509_STORE_CTX* store)
{
    return X509_STORE_CTX_get0_cert(store);
}
#else
X509* certFromStore(X509_STORE_CTX* store)
{
    return store->cert;
}
#endif

std::vector<unsigned char> getCertificate(X509_STORE_CTX* store)
{
    std::vector<unsigned char> hash;
    const EVP_MD* sha256 = EVP_sha256();
    if (store && sha256)
    {
        X509* certificate = certFromStore(store);
        if (certificate)
        {
            hash.resize(EVP_MAX_MD_SIZE);
            unsigned int length = hash.size();
            if (X509_digest(certificate, sha256, hash.data(), &length) == 1)
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

}  // anon namespace

VerifyCache::VerifyCache(openssl::Context::Verifier verifier, int timeout) :
    m_verifier(verifier),
    m_timeout(timeout),
    m_cache(),
    m_expiry()
{ }

int VerifyCache::forwardVerify(X509_STORE_CTX* context)
{
    int result = 0;
    if (m_verifier)
    {
        result = m_verifier(context);
    }
    return result;
}

int VerifyCache::verify(X509_STORE_CTX* context)
{
    int result = 0;

    auto currentHash = getCertificate(context);
    auto now = std::chrono::steady_clock::now();
    if (now > m_expiry)
    {
        // Cache has expired
        m_cache.clear();
    }

    // Cache is valid, check the certificate
    if (!currentHash.empty() && currentHash == m_cache)
    {
        result = 1;
    }
    else
    {
        result = forwardVerify(context);
        if (result == 1)
        {
            // Cache the new result
            m_cache = std::move(currentHash);
            m_expiry = now + std::chrono::seconds(m_timeout);
        }
    }

    return result;
}

}  // namespace dote
