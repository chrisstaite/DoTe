
#include "openssl/context.h"

#include <openssl/ssl.h>

namespace dote {
namespace openssl {

Context::Context() :
    m_context(nullptr)
{
    // Flag to track if we've tried initialising the OpenSSL
    // library yet because we shouldn't keep trying
    static bool initialisationTried = false;
    
    const SSL_METHOD* method = SSLv23_method();
    // If unable to get the method, try initialising the library
    // if we haven't tried to do so already...
    if (method == nullptr && !initialisationTried)
    {
        initialisationTried = true;
        SSL_library_init();
        method = SSLv23_method();
    }

    // Now let's try to create a new context
    if (method != nullptr)
    {
        m_context = SSL_CTX_new(method);
    }
    
    // If the context was created, we need to configure it ready
    // for use
    if (m_context)
    {
        configureContext();
    }
}

void Context::configureContext()
{
    // Disable SSL v2 and v3 so we only use TLS
    const long flags = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;
    SSL_CTX_set_options(m_context, flags);
    
    // We want a certificate, but we'll verify it using a certificate
    // pin rather than using a certificate chain
    SSL_CTX_set_verify(
        m_context, SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr
    );
}

SSL_CTX* Context::get()
{
    return m_context;
}

Context::~Context()
{
    if (m_context)
    {
        SSL_CTX_free(m_context);
    }
}

}  // namespace openssl
}  // namespace dote
