
#include "verify_cache.h"
#include "openssl/certificate_utilities.h"

#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

namespace dote {

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

    openssl::CertificateUtilities utility(context);
    auto currentHash = utility.getHash();
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
