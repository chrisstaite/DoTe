
#pragma once

#include "i_ssl_connection.h"

#include <openssl/ssl.h>

#include <memory>
#include <functional>

namespace dote {
namespace openssl {

class Context;
class CertificateUtilities;

/// \brief  A wrapper around an OpenSSL SSL connection
class SslConnection : public ISslConnection
{
  public:
    /// \brief  Create a new SSL connection
    ///
    /// \param context  The context to create the connection in
    SslConnection(std::shared_ptr<Context> context);

    SslConnection(const SslConnection&) = delete;
    SslConnection& operator=(const SslConnection&) = delete;

    /// \brief  Clean up the wrapped connection
    ~SslConnection() noexcept;

    /// \brief  Disable certificate verification, should be used
    ///         for testing only, it kind of defeats the point
    void disableVerification() override;

    /// \brief  Set the underlying socket for this connection
    ///
    /// \param handle  The underlying socket to set on this connection
    void setSocket(int handle) override;

    /// \brief  Get the SHA-256 hash of the public key of the attached
    ///         peer certificate after connect has completed
    ///
    /// \return  The SHA-256 hash of the certificate's public key
    std::vector<unsigned char> getPeerCertificatePublicKeyHash() override;

    /// \brief  Get the certificate common name for the peer certificate
    ///
    /// \return  The peer certificate common name
    std::string getCommonName() override;

    /// \brief  Connect the underlying connection
    ///
    /// \return  The status of the function
    Result connect() override;

    /// \brief  Shutdown the underlying connection
    ///
    /// \return  The status of the function
    Result shutdown() override;

    /// \brief  Write a buffer to the socket
    ///
    /// \param buffer  The buffer to write
    ///
    /// \return  The status of the function
    Result write(const std::vector<char>& buffer) override;

    /// \brief  Read from the socket
    ///
    /// \param buffer  The buffer to read into, resized to fit the content
    ///
    /// \return  The status of the function
    Result read(std::vector<char>& buffer) override;

    /// \brief  Set the verifier for the connections, by default
    ///         connections are verified by PKI, this allows SPKI
    ///
    /// \param verifier  The verifier to set
    void setVerifier(Verifier verifier) override;

    /// \brief  Verify a connection based on the given certificate store
    ///
    /// \param store  The certificate store to verify
    ///
    /// \return  2 if pin and hostname pass, 1 if hostname only, 0 if not valid
    int verify(X509_STORE_CTX* store);

  private:
    /// The maximum size of the read
    static constexpr std::size_t MAX_FRAME = 16 * 1024;

    /// \brief  Perform a function on the underlying SSL handling the
    ///         non-blocking errors
    ///
    /// \param function  The function to perform
    ///
    /// \return  The result of the function
    Result doFunction(std::function<int(SSL*)> function);

    /// The context that this connection is made under
    std::shared_ptr<Context> m_context;
    /// The wrapped SSL connection
    SSL* m_ssl;
    /// The verifier to use for the connection if not the default
    Verifier m_verifier;
};

}  // namespace openssl
}  // namespace dote
