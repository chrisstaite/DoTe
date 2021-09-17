
#include "log.h"
#include "openssl/spki_verifier.h"
#include "openssl/hostname_verifier.h"
#include "openssl/certificate_utilities.h"

namespace dote {
namespace openssl {

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

}  // anon namespace

SpkiVerifier::SpkiVerifier(ConfigParser::Forwarder& config) :
    m_config(config)
{ }

int SpkiVerifier::verify(X509_STORE_CTX *store) const
{
    int result = 0;
    if (store)
    {
        X509* certificate = certFromStore(store);
        if (certificate)
        {
            // Allow unless we deny specifically
            if (verifyHash(certificate))
            {
                result = 2;
            }
            else if (!m_config.pin.empty())
            {
                Log::notice << "Forwarder certificate hash failure";
            }
            if (verifyHostname(certificate))
            {
                // Only set to 1 if the pin passed or was empty
                if (result == 2 || m_config.pin.empty())
                {
                    result = 1;
                }
            }
            else if (!m_config.host.empty())
            {
                Log::notice << "Forwarder hostname incorrect";
            }
        }
    }
    return result;
}

bool SpkiVerifier::verifyHostname(X509* certificate) const
{
    bool result = false;
    if (!m_config.host.empty() && certificate)
    {
        HostnameVerifier verifier(certificate);
        result = verifier.isValid(m_config.host);
    }
    return result;
}

bool SpkiVerifier::verifyHash(X509 *certificate) const
{
    bool result = false;
    if (!m_config.pin.empty())
    {
        CertificateUtilities utility(certificate);
        result = (utility.getPublicKeyHash() == m_config.pin);
    }
    return result;
}

}  // namespace openssl
}  // namespace dote
