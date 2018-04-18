
#pragma once

#include "openssl/i_ssl_factory.h"

namespace dote {
namespace openssl {

class Context;

/// \brief  A wrapper around an SSL context to create new connections
class SslFactory : public ISslFactory
{
  public:
    /// \brief  Wrap a context to create the SSL connections with
    ///
    /// \param context  The context to use for new SSL connections
    SslFactory(std::shared_ptr<Context> context);

    /// \brief  Required noexcept due to interface
    ~SslFactory() noexcept;

    /// \brief  Create a new SSL connection
    ///
    /// \return  A new SSL connection
    std::shared_ptr<ISslConnection> create() override;

  private:
    /// The context to create the connection for
    std::shared_ptr<Context> m_context;
};

}  // namespace openssl
}  // namespace dote
