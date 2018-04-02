
#pragma once

#include <memory>

typedef struct bio_method_st BIO_METHOD;
typedef struct bio_st BIO;

namespace dote {
namespace openssl {

/// \brief  A wrapper around an OpenSSL BIO
class Bio
{
  public:
    /// \brief  Create a new memory BIO
    ///
    /// \return  A pointer to a new in-memory BIO
    static std::shared_ptr<Bio> memoryBio();

    Bio(const Bio&) = delete;
    Bio& operator=(const Bio&) = delete;

    /// \brief  Destroy the wrapped BIO
    ~Bio();

  private:
    /// \brief  Create a bio of the given type
    ///
    /// \param type  The type of BIO to create
    Bio(BIO_METHOD* type);

    /// \brief  Get access to the raw pointer
    ///
    /// \return  The raw pointer
    BIO* get();

    // Allow access to the raw pointer to the client
    friend class Client;

    /// The created BIO
    BIO* m_bio;
};

}  // namepsace openssl
}  // namespace dote
