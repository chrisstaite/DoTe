
#include "openssl/ssl_connection.h"
#include "openssl/context.h"
#include "openssl/hostname_verifier.h"

#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include <string>
#include <array>

namespace dote {
namespace openssl {

using namespace std::placeholders;

SslConnection::SslConnection(std::shared_ptr<Context> context) :
    m_context(std::move(context)),
    m_ssl(nullptr)
{
    if (m_context && m_context->get())
    {
        m_ssl = SSL_new(m_context->get());
    }
}

SslConnection::SslConnection(SslConnection&& other) :
    m_context(std::move(other.m_context)),
    m_ssl(other.m_ssl)
{
    other.m_ssl = nullptr;
}

SslConnection& SslConnection::operator=(dote::openssl::SslConnection&& other)
{
    std::swap(m_context, other.m_context);
    std::swap(m_ssl, other.m_ssl);
    return *this;
}

SslConnection::~SslConnection()
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

std::vector<unsigned char> SslConnection::getPeerCertificateHash()
{
    std::vector<unsigned char> hash;
    if (m_ssl)
    {
        X509* certificate = SSL_get_peer_certificate(m_ssl);
        const EVP_MD* sha256 = EVP_sha256();
        if (certificate && sha256)
        {
            hash.resize(EVP_MAX_MD_SIZE);
            unsigned int length = hash.size();
            if (X509_pubkey_digest(
                        certificate, sha256, hash.data(), &length
                    ) == 1)
            {
                hash.resize(length);
            }
            else
            {
                hash.clear();
            }
        }
        if (certificate)
        {
            X509_free(certificate);
        }
    }
    return hash;
}

bool SslConnection::verifyHostname(const std::string& hostname)
{
    bool result = false;
    if (m_ssl)
    {
        X509* certificate = SSL_get_peer_certificate(m_ssl);
        if (certificate)
        {
            HostnameVerifier verifier(certificate);
            result = verifier.isValid(hostname);
            X509_free(certificate);
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
            }
        }
    }
    return result;
}

SslConnection::Result SslConnection::connect()
{
    return doFunction(&SSL_connect);
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
