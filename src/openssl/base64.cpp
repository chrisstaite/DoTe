
#include "openssl/base64.h"

#include <openssl/bio.h>
#include <openssl/evp.h>

#include <memory>

namespace dote {
namespace openssl {

std::vector<unsigned char> Base64::decode(const std::string& input)
{
    size_t inputLength = input.length();
    size_t maxOutputLength = (inputLength / 4u) * 3u;

    std::unique_ptr<BIO, decltype(&BIO_free_all)> bio(
        BIO_new_mem_buf(const_cast<char*>(input.data()), input.size()),
        &BIO_free_all
    );
    BIO* b64 = BIO_new(BIO_f_base64());
    if (!b64)
    {
        return { };
    }
    bio.reset(BIO_push(b64, bio.release()));
    BIO_set_flags(bio.get(), BIO_FLAGS_BASE64_NO_NL);

    std::vector<unsigned char> result(maxOutputLength);
    result.resize(BIO_read(bio.get(), result.data(), maxOutputLength));
    return result;
}

}  // namespace openssl
}  // namespace dote
