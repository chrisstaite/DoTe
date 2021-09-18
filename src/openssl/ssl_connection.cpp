
#include "openssl/ssl_connection.h"
#include "openssl/context.h"
#include "openssl/certificate_utilities.h"

#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include <string>
#include <array>

namespace dote {
namespace openssl {

using namespace std::placeholders;

namespace {

/// \brief  Perform a certificate utility function on the peer certificate
///
/// \param utility  The utility to run
/// \param ssl  The connection to get the peer certificate from
///
/// \return  The result of the utility on the peer certificate or default initialised on error
template<typename T>
T certificateOperation(T(CertificateUtilities::*utility)(), SSL* ssl)
{
    T result;
    if (ssl)
    {
        std::unique_ptr<X509, decltype(&X509_free)> certificate(
            SSL_get_peer_certificate(ssl), &X509_free
        );
        if (certificate)
        {
            CertificateUtilities utilities(certificate.get());
            result = (utilities.*utility)();
        }
    }
    return result;
}

}  // anon namespace

SslConnection::SslConnection(std::shared_ptr<Context> context) :
    m_context(std::move(context)),
    m_ssl(nullptr),
    m_verifier()
{
    if (m_context)
    {
        m_ssl = SSL_new(m_context->get());
        m_context->setSslConnection(m_ssl, this);
        SSL_SESSION* session = m_context->getSession();
        if (m_ssl && session)
        {
            SSL_set_session(m_ssl, session);
        }
    }
}

SslConnection::~SslConnection() noexcept
{
    if (m_context)
    {
        // Probably not required because we free the m_ssl,
        // but it's better safe than sorry...
        m_context->setSslConnection(m_ssl, nullptr);
    }
    if (m_ssl)
    {
        SSL_free(m_ssl);
    }
}

void SslConnection::setSocket(int handle)
{
    if (m_ssl && handle > 0)
    {
        SSL_set_fd(m_ssl, handle);
    }
}



std::vector<unsigned char> SslConnection::getPeerCertificatePublicKeyHash()
{
    return certificateOperation(&CertificateUtilities::getPublicKeyHash, m_ssl);
}

std::string SslConnection::getCommonName()
{
    return certificateOperation(&CertificateUtilities::getCommonName, m_ssl);
}

void SslConnection::disableVerification()
{
    if (m_ssl)
    {
        SSL_set_verify(m_ssl, SSL_VERIFY_NONE, nullptr);
    }
}

SslConnection::Result SslConnection::doFunction(std::function<int(SSL*)> function)
{
    Result result = Result::FATAL;
    if (m_ssl)
    {
        int ret = function(m_ssl);
        if (ret > 0)
        {
            result = Result::SUCCESS;
        }
        else if (ret == 0)
        {
            result = Result::CLOSED;
        }
        else
        {
            ret = SSL_get_error(m_ssl, ret);
            switch (ret)
            {
                case SSL_ERROR_WANT_READ:
                    result = Result::NEED_READ;
                    break;
                case SSL_ERROR_WANT_WRITE:
                    result = Result::NEED_WRITE;
                    break;
                case SSL_ERROR_ZERO_RETURN:
                    result = Result::CLOSED;
                    break;
            }
        }
    }
    return result;
}

SslConnection::Result SslConnection::connect()
{
    Result result = doFunction(&SSL_connect);
    if (result == Result::SUCCESS)
    {
        SSL_SESSION* session = SSL_get1_session(m_ssl);
        if (session)
        {
            m_context->cacheSession(session);
        }
    }
    else if (result == Result::FATAL)
    {
        // Going to change servers, invalidate the session
        m_context->cacheSession(nullptr);
    }
    return result;
}

SslConnection::Result SslConnection::shutdown()
{
    return doFunction(&SSL_shutdown);
}

SslConnection::Result SslConnection::write(const std::vector<char>& buffer)
{
    if (m_ssl && buffer.size() == 0)
    {
        // Undefined behaviour of SSL_write when size is zero
        return Result::SUCCESS;
    }

    return doFunction(
        std::bind(&SSL_write, _1, buffer.data(), buffer.size())
    );
}

SslConnection::Result SslConnection::read(std::vector<char>& buffer)
{
    int readLength = 0;
    char stackBuffer[MAX_FRAME];
    Result result = doFunction(
        [&readLength, &stackBuffer](SSL* ssl)
        {
            readLength = SSL_read(ssl, stackBuffer, sizeof(stackBuffer));
            return readLength;
        }
    );
    if (result == Result::SUCCESS)
    {
        buffer.resize(readLength);
        std::copy(stackBuffer, &stackBuffer[readLength], buffer.begin());
    }
    return result;
}

void SslConnection::setVerifier(Context::Verifier verifier)
{
    m_verifier = std::move(verifier);
}

int SslConnection::verify(X509_STORE_CTX *store)
{
    // If there's no verifier set, then use the internal verification
    int result = 1;
    if (m_verifier)
    {
        result = m_verifier(store);
    }
    return result;
}

}  // namespace openssl
}  // namespace dote
