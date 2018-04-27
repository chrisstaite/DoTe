
#pragma once

#include "openssl/context.h"

#include <chrono>
#include <vector>

namespace dote {

/// \brief  A class that is able to cache a verified certificate
///         for a given amount of time so it doesn't need to verify
///         the same certificate the next time it sees it
class VerifyCache
{
  public:
    /// \brief  Initialise a cache
    ///
    /// \param verifier  The actual verifier to use
    /// \param timeout   The timeout of the cache in seconds
    VerifyCache(openssl::Context::Verifier verifier, int timeout);

    VerifyCache(const VerifyCache&) = delete;
    VerifyCache& operator=(const VerifyCache&) = delete;

    /// \brief  Verify a certificate context
    ///
    /// \param context  The context to verify
    ///
    /// \return  1 if the certificate is in the cache or verified
    ///          by the forwarded verifier or 0 if not
    int verify(X509_STORE_CTX* context);

  private:
    /// \brief  Forward the verification on to m_verifier and cache
    ///         if it is successful
    ///
    /// \param context  The context to forward to the verifier
    ///
    /// \return  The result of the forward verifier
    int forwardVerify(X509_STORE_CTX* context);
  
    /// The verifier to pass on to
    openssl::Context::Verifier m_verifier;
    /// The number of seconds to cache for
    int m_timeout;
    /// The cached certificate
    std::vector<unsigned char> m_cache;
    /// The time the cache expires
    std::chrono::steady_clock::time_point m_expiry;
};

}  // namespace dote
