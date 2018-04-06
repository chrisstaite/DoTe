
#include "openssl/ssl_connection.h"
#include "openssl/context.h"
#include "openssl/hostname_verifier.h"

#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include <string>

namespace dote {
namespace openssl {

namespace {

/// \brief  Base64 encode a buffer
///
/// \param data    The buffer to encode
/// \param length  The length of the buffer
///
/// \return  The base64 encoding or empty on error
std::string base64(unsigned char* data, unsigned int length)
{
    BIO* b64 = BIO_new(BIO_f_base64());
    if (!b64)
    {
        return { };
    }

    BIO* bmem = BIO_new(BIO_s_mem());
    if (!bmem)
    {
        BIO_free_all(b64);
        return { };
    }

    b64 = BIO_push(b64, bmem);
    BIO_write(b64, data, length);
    BIO_flush(b64);

    BUF_MEM *bptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string result(bptr->data, bptr->length - 1);
    BIO_free_all(b64);

    return result;
}

}  // anon namespace

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
    if (m_ssl && handle >= 0)
    {
        SSL_set_fd(m_ssl, handle);
    }
}

std::string SslConnection::getPeerCertificateHash()
{
    std::string hash;
    if (m_ssl)
    {
        X509* certificate = SSL_get_peer_certificate(m_ssl);
        const EVP_MD* sha256 = EVP_sha256();
        if (certificate && sha256)
        {
            unsigned char md[EVP_MAX_MD_SIZE];
            unsigned int length = sizeof(md);
            if (X509_pubkey_digest(certificate, sha256, md, &length) == 1)
            {
                hash = base64(md, length);
            }
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
    return doFunction(SSL_shutdown);
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
