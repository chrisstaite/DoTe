
#include "openssl/wrapped_bio.h"

#include <openssl/bio.h>

namespace dote {
namespace openssl {

Bio::Bio(BIO_METHOD* type) :
    m_bio(BIO_new(type))
{ }

std::shared_ptr<Bio> Bio::memoryBio()
{
    return std::shared_ptr<Bio>(new Bio(BIO_s_mem()));
}

Bio::~Bio()
{
    if (m_bio)
    {
        BIO_free(m_bio);
    }
}

}  // namespace openssl
}  // namespace dote
