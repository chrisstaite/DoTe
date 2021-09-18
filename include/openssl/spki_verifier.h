
#include "config_parser.h"

#include <vector>
#include <string>

#include <openssl/ssl.h>

namespace dote {
namespace openssl {

class CertificateUtilities;

class SpkiVerifier
{
  public:
    /// \brief  Create a verifier for a given forwarder
    ///
    /// \param config  The forwarder to create the verifier for, lifetime must outlive this instance
    explicit SpkiVerifier(ConfigParser::Forwarder& config);

    /// \brief  Verify a certificate matches the SPKI requirements
    ///
    /// \param store  The store to get the certificates to check
    ///
    /// \return  2 if pin and hostname pass, 1 if hostname only, 0 if not valid
    int verify(X509_STORE_CTX* store) const;

  private:
    /// \brief  Check the given peer certificate is valid for the
    ///         configured hostname
    ///
    /// \param certificate  The certificate to verify against the hostname
    ///
    /// \return  True if the hostname is valid for the connected peer
    bool verifyHostname(X509* certificate) const;

    /// \brief  Check the given peer certificate is valid for the
    ///         configured public key hash
    ///
    /// \param certificate  The certificate to verify against the hash
    ///
    /// \return  True if the SHA-256 hash of the public key matches the configured hash
    bool verifyHash(CertificateUtilities& certificate) const;

    /// The forwarder configuration to validate against
    ConfigParser::Forwarder& m_config;
};

}  // namespace openssl
}  // namespace dote
