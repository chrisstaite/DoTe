
#include "openssl/client.h"
#include "openssl/wrapped_bio.h"
#include "openssl/context.h"

#include <openssl/ssl.h>

namespace dote {
namespace openssl {

Client::Client(std::shared_ptr<Context> context,
               std::shared_ptr<Bio> read,
               std::shared_ptr<Bio> write) :
    m_context(std::move(context)),
    m_read(std::move(read)),
    m_write(std::move(write)),
    m_ssl(nullptr)
{
    if (m_context && m_context->get() &&
            m_read && m_read->get() &&
            m_write && m_write->get())
    {
        m_ssl = SSL_new(m_context->get());
        if (m_ssl)
        {
            // Place in client mode
            SSL_set_connect_state(m_ssl);
            // Configure the input and output BIOs
            SSL_set_bio(m_ssl, read->get(), write->get());
        }
    }
}

Client::~Client()
{
    if (m_ssl)
    {
        SSL_free(m_ssl);
    }
}

}  // namespace openssl
}  // namespace dote
