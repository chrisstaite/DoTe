
#pragma once

#include <string>

typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_session_st SSL_SESSION;

namespace dote {
namespace openssl {

/// \brief  A wrapper around an OpenSSL context which automatically
///         cleans it up on destruction.  Will also initialise the
///         library on construction if context creation fails.
class Context {
  public:
    /// \brief  Create a new OpenSSL context, initialising the library
    ///         if required.
    ///
    /// \param ciphers  The ciphers to use with OpenSSL
    Context(const std::string& ciphers);
    
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    
    /// \brief  Clean up the context
    ~Context();

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

    /// The wrapped context
    SSL_CTX* m_context;
    /// The client session that we could re-use
    SSL_SESSION* m_session;
};

}  // namespace openssl
}  // namespace dote
