
#include "openssl/base64.h"

#include <openssl/bio.h>
#include <openssl/evp.h>

#include <cstdio>
#include <memory>

namespace dote {
namespace openssl {

std::vector<unsigned char> Base64::decode(const std::string& input)
{
    size_t inputLength = input.length();
    size_t maxOutputLength = (inputLength / 4u) * 3u;

    std::vector<unsigned char> result;
    result.resize(maxOutputLength);
    std::unique_ptr<FILE, decltype(&fclose)> inputFile(
        fmemopen(const_cast<char*>(input.data()), inputLength, "r"),
        &fclose
    );
    if (!inputFile)
    {
        return { };
    }
    std::unique_ptr<BIO, decltype(&BIO_free_all)> bio(
        BIO_new(BIO_f_base64()), &BIO_free_all
    );
    BIO* bmem = BIO_new_fp(inputFile.get(), BIO_NOCLOSE);
    if (!bmem)
    {
        return { };
    }
    bio.reset(BIO_push(bio.release(), bmem));
    BIO_set_flags(bio.get(), BIO_FLAGS_BASE64_NO_NL);
    result.resize(BIO_read(
        bio.get(), result.data(), maxOutputLength
    ));

    return result;
}

}  // namespace openssl
}  // namespace dote
