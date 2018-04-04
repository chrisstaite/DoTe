
#pragma once

#include <memory>
#include <vector>
#include <functional>

typedef struct ssl_st SSL;

namespace dote {
namespace openssl {

class Context;

/// \brief  A wrapper around an OpenSSL SSL connection
class SslConnection
{
  public:
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
        /// There's no way that this is going to succeed,
        /// don't bother trying to call again
        FATAL
    };

    /// \brief  Create a new SSL connection
    ///
    /// \param context  The context to create the connection in
    SslConnection(std::shared_ptr<Context> context);

    /// \brief  Move the connection
    ///
    /// \param other  The connection to move
    SslConnection(SslConnection&& other);

    /// \brief  Move the connection
    ///
    /// \param other  The connection to move
    ///
    /// \return  A reference to this instance
    SslConnection& operator=(SslConnection&& other);

    SslConnection(const SslConnection&) = delete;
    SslConnection& operator=(const SslConnection&) = delete;

    /// \brief  Clean up the wrapped connection
    ~SslConnection();

    /// \brief  Set the hostname to verify, must be called before
    ///         calling connect() otherwise it won't be verified
    ///
    /// \param hostname  The hostname to verify against
    ///
    /// \return  True if the hostname verification was successfully
    ///          added to the context
    bool setHostname(const std::string& hostname);

    /// \brief  Set the underlying socket for this connection
    ///
    /// \param handle  The underlying socket to set on this connection
    void setSocket(int handle);

    /// \brief  Get the Base64 encoding of the SHA-256 hash of the
    ///         attached peer certificate after connect has completed
    ///
    /// \return  The Base64 encoding of the peer certificate
    std::string getPeerCertificateHash();

    /// \brief  Connect the underlying connection
    ///
    /// \return  The status of the function
    Result connect();

    /// \brief  Shutdown the underlying connection
    ///
    /// \return  The status of the function
    Result shutdown();

    /// \brief  Write a buffer to the socket
    ///
    /// \param buffer  The buffer to write
    ///
    /// \return  The status of the function
    Result write(const std::vector<char>& buffer);

    /// \brief  Read from the socket
    ///
    /// \param buffer  The buffer to read into, resized to fit the content
    ///
    /// \return  The status of the function
    Result read(std::vector<char>& buffer);

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
};

}  // namespace openssl
}  // namespace dote
