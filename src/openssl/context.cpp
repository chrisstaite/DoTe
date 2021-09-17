
#include "log.h"
#include "openssl/context.h"
#include "openssl/ssl_connection.h"

#include <openssl/ssl.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/conf.h>

namespace dote {
namespace openssl {

namespace {

/// The index that the Context pointer is stored in the SSLConnection*
int s_connectionIndex;

/// A list of errors that are allowed if there is a verifier set and it passes
/// These errors are allowed because the verifier performs SPKI
constexpr int ALLOWED_ERRORS[] = {
    // The CA was not verified
    X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT,
    // The certificate was self signed
    X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT,
    // One of the certificates were self signed
    X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN,
    // Missing a certificate in the chain
    X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY,
    // Missing the chain
    X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE
};

}  // anon namespace

Context::Context(const std::string& ciphers) :
    m_context(nullptr),
    m_session(nullptr),
    m_chainVerifier()
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
        s_connectionIndex =
            SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
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

void Context::setChainVerifier(Verifier verifier)
{
    if (m_context)
    {
        m_chainVerifier = std::move(verifier);
        SSL_CTX_set_cert_verify_callback(
            m_context, &Context::chainVerifyTrampoline, this
        );
    }
}

void Context::setSslConnection(SSL* ssl, SslConnection* connection)
{
    if (m_context && ssl)
    {
        SSL_set_ex_data(ssl, s_connectionIndex, connection);
    }
}

int Context::verifyTrampoline(int preverify, X509_STORE_CTX* store)
{
    // Find the context from the store
    auto ssl = reinterpret_cast<SSL*>(
        X509_STORE_CTX_get_ex_data(store, SSL_get_ex_data_X509_STORE_CTX_idx())
    );
    auto connection = (ssl == nullptr) ? nullptr :
        reinterpret_cast<SslConnection*>(SSL_get_ex_data(ssl, s_connectionIndex));

    // If there was an error, check it was one that SPKI can fix
    bool allowedError = (preverify == 1);
    int error = X509_STORE_CTX_get_error(store);
    for (size_t i = 0;
         false == allowedError &&
            i < sizeof(ALLOWED_ERRORS) / sizeof(ALLOWED_ERRORS[0]);
         ++i)
    {
        if (ALLOWED_ERRORS[i] == error)
        {
            allowedError = true;
        }
    }

    // This will get called if there was no error, or if there was an error but
    // it is one that can be remedied by SPKI
    if (allowedError && connection)
    {
        // 2 if pin and hostname pass, 1 if hostname only, 0 if not valid
        int ret = connection->verify(store);
        if (ret == 2)
        {
            // Override of the failure
            preverify = 1;
        }
        else if (ret == 0)
        {
            // Override of the (possible) pass
            preverify = 0;
        }
    }

    return preverify;
}

int Context::chainVerifyTrampoline(X509_STORE_CTX *store, void *context)
{
    int result = 0;
    auto sslContext = reinterpret_cast<Context*>(context);
    if (sslContext && sslContext->m_chainVerifier)
    {
        result = sslContext->m_chainVerifier(store);
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
    // We're sending tiny packets, so it doesn't buy much
    flags |= SSL_OP_NO_COMPRESSION;
    SSL_CTX_set_options(m_context, flags);

    // Use the default verification paths
    SSL_CTX_set_default_verify_paths(m_context);

    // By default perform verification using standard OpenSSL routines
    SSL_CTX_set_verify(m_context, SSL_VERIFY_PEER, &Context::verifyTrampoline);
}

SSL_CTX* Context::get()
{
    return m_context;
}

}  // namespace openssl
}  // namespace dote
