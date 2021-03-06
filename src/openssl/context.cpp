
#include "log.h"
#include "openssl/context.h"

#include <openssl/ssl.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/conf.h>

namespace dote {
namespace openssl {

Context::Context(const std::string& ciphers) :
    m_context(nullptr),
    m_session(nullptr)
{
    // Flag to track if we've tried initialising the OpenSSL
    // library yet because we shouldn't keep trying
    static bool initialisationTried = false;
    
    // Let's try to create a new context
    const SSL_METHOD* method = SSLv23_method();
    if (method != nullptr)
    {
        m_context = SSL_CTX_new(method);
    }

    // If unable to create a context, try initialising the library
    // if we haven't tried to do so already...
    if (m_context == nullptr && !initialisationTried)
    {
        initialisationTried = true;
        SSL_library_init();
        m_context = SSL_CTX_new(method);
    }

    // If the context was created, we need to configure it ready
    // for use
    if (m_context)
    {
        // Set the available ciphers
        if (SSL_CTX_set_cipher_list(m_context, ciphers.c_str()) == 0)
        {
            Log::err << "Unable to set the OpenSSL cipher list";
            SSL_CTX_free(m_context);
            m_context = nullptr;
        }
        else
        {
            configureContext();
        }
    }
    else
    {
        Log::err << "Unable to create OpenSSL context";
    }
}

Context::~Context()
{
    cacheSession(nullptr);
    if (m_context)
    {
        SSL_CTX_free(m_context);
        
        FIPS_mode_set(0);
        ENGINE_cleanup();
        CONF_modules_unload(1);
        EVP_cleanup();
        CRYPTO_cleanup_all_ex_data();
        ERR_free_strings();
    }
}

void Context::setVerifier(Verifier verifier)
{
    if (m_context)
    {
        m_verifier = std::move(verifier);
        SSL_CTX_set_cert_verify_callback(
            m_context, &Context::verifyTrampoline, this
        );
    }
}

int Context::verifyTrampoline(X509_STORE_CTX* store, void* context)
{
    int result = 0;
    auto sslContext = reinterpret_cast<Context*>(context);
    if (sslContext && sslContext->m_verifier)
    {
        result = sslContext->m_verifier(store);
    }
    return result;
}

void Context::cacheSession(SSL_SESSION* session)
{
    if (m_session)
    {
        SSL_SESSION_free(m_session);
    }
    m_session = session;
}

SSL_SESSION* Context::getSession()
{
    return m_session;
}

void Context::configureContext()
{
    // Disable SSL v2 and v3 so we only use TLS
    long flags = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;
    // Disable compression to reduce the memory footprint
    flags |= SSL_OP_NO_COMPRESSION;
    SSL_CTX_set_options(m_context, flags);

    // Use the default verification paths
    SSL_CTX_set_default_verify_paths(m_context);

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

}  // namespace openssl
}  // namespace dote
