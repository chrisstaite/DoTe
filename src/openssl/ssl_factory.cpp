
#include "openssl/ssl_factory.h"
#include "openssl/ssl_connection.h"

namespace dote {
namespace openssl {

SslFactory::SslFactory(std::shared_ptr<Context> context) :
    m_context(std::move(context))
{ }

std::shared_ptr<ISslConnection> SslFactory::create()
{
    if (m_context)
    {
        return std::make_shared<SslConnection>(m_context);
    }
    return {nullptr};
}

}  // namespace openssl
}  // namespace dote
