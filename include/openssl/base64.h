
#pragma once

#include <vector>
#include <string>

typedef struct bio_st BIO;

namespace dote {
namespace openssl {

/// \brief  A wrapper around the OpenSSL Base64 BIO
class Base64
{
  public:
    Base64() = delete;

    /// \brief  Decode a Base64 string into a vector
    ///
    /// \param input  The base64 string to decode
    ///
    /// \return  The contents of the decoded string
    static std::vector<unsigned char> decode(const std::string& input);

    /// \brief  Encode a vector into a Base64 string
    ///
    /// \param input  The data to encode into Base64
    ///
    /// \return  The Base64 encoded string
    static std::string encode(const std::vector<unsigned char>& input);
};

}  // namespace openssl
}  // namespace dote
