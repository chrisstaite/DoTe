
#pragma once

#include <openssl/x509_vfy.h>

#include <string>
#include <functional>

typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_session_st SSL_SESSION;

namespace dote {
namespace openssl {

class SslConnection;

/// \brief  A wrapper around an OpenSSL context which automatically
///         cleans it up on destruction.  Will also initialise the
///         library on construction if context creation fails.
class Context
{
  public:
    /// The type of verifier to forward on to, returns 1 on success
    /// and 0 on failure
    using Verifier = std::function<int(X509_STORE_CTX*)>;

    /// \brief  Create a new OpenSSL context, initialising the library
    ///         if required.
    ///
    /// \param ciphers  The ciphers to use with OpenSSL
    Context(const std::string& ciphers);

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    /// \brief  Clean up the context
    ~Context();

    /// \brief  Set the chain verifier for all incoming connections
    ///
    /// \prarm verifier  The verifier to set
    void setChainVerifier(Verifier verifier);

    /// \brief  Set the SSLConnection instance for the SSL object, this
    ///         causes the verify function to be fired
    ///
    /// \param ssl  The SSL object to get the connection for
    /// \param connection  The connection to set on the SSL object
    void setSslConnection(SSL* ssl, SslConnection* connection);

  protected:
    /// \brief  Get the raw context
    ///
    /// \return  The raw context
    SSL_CTX* get();

    /// \brief  Cache the session, overwriting any existing one
    ///
    /// \param session  The session to cache
    void cacheSession(SSL_SESSION* session);

    /// \brief  Get the cached session
    ///
    /// \return  The cached session or nullptr if none
    SSL_SESSION* getSession();

    /// Allow the connection access to the raw context
    friend class SslConnection;

  private:
    /// \brief  If the context has been created, initialise it ready
    ///         for use.
    void configureContext();

    /// \brief  A C-style trampoline to get back the C++ instance to
    ///         perform the certificate verification
    ///
    /// \param preverify  Set to 0 if the chain failed verification built-in, 1 otherwise
    /// \param store      The context to verify
    ///
    /// \return  The result of the context->m_verifier function or
    ///          0 if any of the checks fail
    static int verifyTrampoline(int preverify, X509_STORE_CTX* store);

    /// \brief  A C-style trampoline to get back the C++ instance to
    ///         perform the certificate verification
    ///
    /// \param store      The context to verify
    /// \param context  The Context instance to forward to
    ///
    /// \return  The result of the context->m_chainVerifier function or
    ///          0 if any of the checks fail
    static int chainVerifyTrampoline(X509_STORE_CTX* store, void* context);

    /// The wrapped context
    SSL_CTX* m_context;
    /// The client session that we could re-use
    SSL_SESSION* m_session;
    /// The chain verifier to use for the connection if not the default
    Verifier m_chainVerifier;
};

}  // namespace openssl
}  // namespace dote
