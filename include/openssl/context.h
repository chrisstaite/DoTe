
#pragma once

typedef struct ssl_ctx_st SSL_CTX;

namespace dote {
namespace openssl {

/// \brief  A wrapper around an OpenSSL context which automatically
///         cleans it up on destruction.  Will also initialise the
///         library on construction if context creation fails.
class Context {
  public:
    /// \brief  Create a new OpenSSL context, initialising the library
    ///         if required.
    Context();
    
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    
    /// \brief  Clean up the context
    ~Context();
    
  private:
    /// \brief  If the context has been created, initialise it ready
    ///         for use.
    void configureContext();

    /// \brief  Get the raw context
    ///
    /// \return  The raw context
    SSL_CTX* get();
    
    // Allow the client access to the raw context
    friend class Client;
  
    /// The wrapped context
    SSL_CTX* m_context;
};

}  // namespace openssl
}  // namespace dote
