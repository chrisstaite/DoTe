
#include <openssl/ssl.h>

#include <vector>
#include <string>

namespace dote {
namespace openssl {

class CertificateUtilities
{
  public:
    /// \brief  Create a utilty instance for the given certificate
    ///
    /// \param certificate  A certificate to work on, must last the lifetime of the instance
    explicit CertificateUtilities(X509* certificate);

    /// \brief  Get the SHA-256 hash of the public key of the given certificate
    ///
    /// \return  The SHA-256 hash of the certificate's public key or empty vector on error
    std::vector<unsigned char> getPublicKeyHash();

    /// \brief  Get the common name of the certificate
    ///
    /// \return  The common name of the certificate or empty string on error
    std::string getCommonName();

  private:
    /// The hash function for getPeerCertificateHash
    using HashFunction = int(*)(
        const X509*, const EVP_MD*, unsigned char*, unsigned int*
    );

    /// \brief  Get the SHA-256 hash of the certificate
    ///
    /// \param function  The hash function to use for the certificate
    ///
    /// \return  The SHA-256 hash
    std::vector<unsigned char> getCertificateHash(HashFunction function);

    /// The certificate to perform operations on
    X509* m_certificate;
};

}  // namespace openssl
}  // namespace dote
