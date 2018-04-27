
#pragma once

#include "i_ssl_connection.h"

#include <memory>
#include <functional>

typedef struct ssl_st SSL;
typedef struct x509_st X509;
typedef struct env_md_st EVP_MD;

namespace dote {
namespace openssl {

class Context;

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

    /// \brief  Set the underlying socket for this connection
    ///
    /// \param handle  The underlying socket to set on this connection
    void setSocket(int handle) override;

    /// \brief  Get the SHA-256 hash of the peer certificate after
    ///         connect has completed
    ///
    /// \return  The SHA-256 hash of the certificate
    std::vector<unsigned char> getPeerCertificateHash() override;

    /// \brief  Get the SHA-256 hash of the public key of the attached
    ///         peer certificate after connect has completed
    ///
    /// \return  The SHA-256 hash of the certificate's public key
    std::vector<unsigned char> getPeerCertificatePublicKeyHash() override;

    /// \brief  Check the connected peer certificate is valid for the
    ///         given hostname after connect has completed
    ///
    /// \param hostname  The hostname to verify
    ///
    /// \return  True if the hostname is valid for the connected peer
    bool verifyHostname(const std::string& hostname) override;

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

  private:
    /// The maximum size of the read
    static constexpr std::size_t MAX_FRAME = 16 * 1024;

    /// The hash function for getPeerCertificateHash
    using HashFunction = int(*)(
        const X509*, const EVP_MD*, unsigned char*, unsigned int*
    );

    /// \brief  Get the SHA-256 hash of the peer certificate
    ///
    /// \param function  The hash function to use for the certificate
    ///
    /// \return  The SHA-256 hash
    std::vector<unsigned char> getPeerCertificateHash(HashFunction function);

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
};

}  // namespace openssl
}  // namespace dote
