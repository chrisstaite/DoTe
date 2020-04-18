
#include "openssl/ssl_connection.h"
#include "openssl/context.h"
#include "openssl/hostname_verifier.h"

#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include <string>
#include <array>

namespace dote {
namespace openssl {

namespace {

std::string toString(ASN1_STRING* string)
{
    if (string)
    {
        return std::string(
            reinterpret_cast<const char*>(
#if OPENSSL_VERSION_NUMBER >= 0x010100000
                ASN1_STRING_get0_data(string)
#else
                ASN1_STRING_data(string)
#endif
            ),
            ASN1_STRING_length(string)
        );
    }
    return {};
}

}  // anon namespace

using namespace std::placeholders;

SslConnection::SslConnection(std::shared_ptr<Context> context) :
    m_context(std::move(context)),
    m_ssl(nullptr)
{
    if (m_context)
    {
        m_ssl = SSL_new(m_context->get());
        SSL_SESSION* session = m_context->getSession();
        if (session)
        {
            SSL_set_session(m_ssl, session);
        }
    }
}

SslConnection::~SslConnection() noexcept
{
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

std::vector<unsigned char> SslConnection::getPeerCertificateHash(HashFunction function)
{
    std::vector<unsigned char> hash;
    if (m_ssl)
    {
        std::unique_ptr<X509, decltype(&X509_free)> certificate(
            SSL_get_peer_certificate(m_ssl), &X509_free
        );
        const EVP_MD* sha256 = EVP_sha256();
        if (certificate && sha256)
        {
            hash.resize(EVP_MAX_MD_SIZE);
            unsigned int length = hash.size();
            if (function(certificate.get(), sha256, hash.data(), &length) == 1)
            {
                hash.resize(length);
            }
            else
            {
                hash.clear();
            }
        }
    }
    return hash;
}

std::vector<unsigned char> SslConnection::getPeerCertificateHash()
{
    return getPeerCertificateHash(&X509_digest);
}

std::vector<unsigned char> SslConnection::getPeerCertificatePublicKeyHash()
{
    return getPeerCertificateHash(&X509_pubkey_digest);
}

std::string SslConnection::getCommonName()
{
    std::string result;
    if (m_ssl)
    {
        std::unique_ptr<X509, decltype(&X509_free)> certificate(
            SSL_get_peer_certificate(m_ssl), &X509_free
        );
        if (certificate)
        {
            int index = X509_NAME_get_index_by_NID(
                X509_get_subject_name(certificate.get()),
                NID_commonName,
                -1
            );
            if (index >= 0)
            {
                X509_NAME_ENTRY* commonName = X509_NAME_get_entry(
                    X509_get_subject_name(certificate.get()), index
                );
                if (commonName)
                {
                    result = toString(X509_NAME_ENTRY_get_data(commonName));
                }
            }
        }
    }
    return result;
}

bool SslConnection::verifyHostname(const std::string& hostname)
{
    bool result = false;
    if (m_ssl)
    {
        std::unique_ptr<X509, decltype(&X509_free)> certificate(
            SSL_get_peer_certificate(m_ssl), &X509_free
        );
        if (certificate)
        {
            HostnameVerifier verifier(certificate.get());
            result = verifier.isValid(hostname);
        }
    }
    return result;
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

}  // namespace openssl
}  // namespace dote
