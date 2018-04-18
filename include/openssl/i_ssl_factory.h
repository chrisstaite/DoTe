
#pragma once

#include "openssl/i_ssl_connection.h"

#include <memory>

namespace dote {
namespace openssl {

/// \brief  An interface to create an SSL connection
class ISslFactory
{
  public:
    virtual ~ISslFactory() = default;

    /// \brief  Create a new SSL connection
    ///
    /// \return  The new SSL connection
    virtual std::shared_ptr<ISslConnection> create() = 0;
};

}  // namespace openssl
}  // namespace dote
