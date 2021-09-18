
#include "log.h"
#include "openssl/spki_verifier.h"
#include "openssl/hostname_verifier.h"
#include "openssl/certificate_utilities.h"

namespace dote {
namespace openssl {

SpkiVerifier::SpkiVerifier(ConfigParser::Forwarder& config) :
    m_config(config)
{ }

int SpkiVerifier::verify(X509_STORE_CTX *store) const
{
    int result = 0;
    // Allow unless we deny specifically
    CertificateUtilities utility(store);
    if (verifyHash(utility))
    {
        result = 2;
    }
    else if (!m_config.pin.empty())
    {
        Log::notice << "Forwarder certificate hash failure";
    }
    if (verifyHostname(utility.certificate()))
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

bool SpkiVerifier::verifyHash(CertificateUtilities& utility) const
{
    bool result = false;
    if (!m_config.pin.empty())
    {
        result = (utility.getPublicKeyHash() == m_config.pin);
    }
    return result;
}

}  // namespace openssl
}  // namespace dote
