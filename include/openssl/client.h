
#pragma once

#include <memory>

typedef struct ssl_st SSL;

namespace dote {
namespace openssl {

class Context;
class Bio;

/// \brief  A wrapper around the OpenSSL SSL object using in-memory
///         BIOs in order to provide a non-blocking implementation
class Client
{
  public:
    /// \brief  Construct an OpenSSL client
    ///
    /// \param context  The context to create the client with
    /// \param read     The BIO to read from the TLS server from
    /// \param write    The BIO to write to the TLS server with
    Client(
        std::shared_ptr<Context> context,
        std::shared_ptr<Bio> read,
        std::shared_ptr<Bio> write);
    
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    
    /// \brief  Deconstruct the wrapped client
    ~Client();
    
  private:
    /// The context that the SSL context is created under
    std::shared_ptr<Context> m_context;
    /// The BIO for reading from the TLS server
    std::shared_ptr<Bio> m_read;
    /// The BIO to write to the TLS server
    std::shared_ptr<Bio> m_write;
    /// The wrapped SSL context
    SSL* m_ssl;
};

}  // namespace openssl
}  // namespace dote
