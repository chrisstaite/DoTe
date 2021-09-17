
#pragma once

#include <vector>
#include <string>
#include <functional>

typedef struct x509_store_ctx_st X509_STORE_CTX;

namespace dote {
namespace openssl {

class ISslConnection
{
  public:
    /// The type of verifier to forward on to, returns 2 if pin and hostname pass, 1 if hostname only, 0 if not valid
    using Verifier = std::function<int(X509_STORE_CTX*)>;

    /// \brief  The result types from communications
    enum Result
    {
        /// Call the function again when there's something
        /// to read on the socket
        NEED_READ,
        /// Call the function again when there's room to
        /// write on the socket
        NEED_WRITE,
        /// The function has completed its role
        SUCCESS,
        /// The remote end has closed
        CLOSED,
        /// There's no way that this is going to succeed,
        /// don't bother trying to call again
        FATAL
    };

    virtual ~ISslConnection() = default;

    /// \brief  Set the underlying socket for this connection
    ///
    /// \param handle  The underlying socket to set on this connection
    virtual void setSocket(int handle) = 0;

    /// \brief  Disable certificate verification, should be used
    ///         for testing only, it kind of defeats the point
    virtual void disableVerification() = 0;

    /// \brief  Set the verifier for the connections, by default
    ///         connections are verified by PKI, this allows SPKI
    ///
    /// \param verifier  The verifier to set
    virtual void setVerifier(Verifier verifier) = 0;

    /// \brief  Get the SHA-256 hash of the public key of the attached
    ///         peer certificate after connect has completed
    ///
    /// \return  The SHA-256 hash of the certificate's public key
    virtual std::vector<unsigned char> getPeerCertificatePublicKeyHash() = 0;

    /// \brief  Get the certificate common name for the peer certificate
    ///
    /// \return  The peer certificate common name
    virtual std::string getCommonName() = 0;

    /// \brief  Connect the underlying connection
    ///
    /// \return  The status of the function
    virtual Result connect() = 0;

    /// \brief  Shutdown the underlying connection
    ///
    /// \return  The status of the function
    virtual Result shutdown() = 0;

    /// \brief  Write a buffer to the socket
    ///
    /// \param buffer  The buffer to write
    ///
    /// \return  The status of the function
    virtual Result write(const std::vector<char>& buffer) = 0;

    /// \brief  Read from the socket
    ///
    /// \param buffer  The buffer to read into, resized to fit the content
    ///
    /// \return  The status of the function
    virtual Result read(std::vector<char>& buffer) = 0;
};

}  // namespace openssl
}  // namespace dote
