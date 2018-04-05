
#pragma once

#include <string>

typedef struct x509_st X509;
typedef struct X509_name_entry_st X509_NAME_ENTRY;
typedef struct asn1_string_st ASN1_STRING;
struct stack_st_GENERAL_NAME;

namespace dote {
namespace openssl {

/// \brief  A class to check whether a hostname is permitted
///         by a given certificate.  This is required as the
///         functionality was only included in OpenSSL 1.0.2+
class HostnameVerifier
{
  public:
    /// \brief  Construct a verifier for a given certificate
    ///
    /// \param certificate  The certificate to verify the
    ///                     hostname against
    HostnameVerifier(X509* certificate);
    
    HostnameVerifier(const HostnameVerifier&) = delete;
    HostnameVerifier& operator=(const HostnameVerifier&) = delete;

    /// \brief  Release the SAN
    ~HostnameVerifier();

    /// \brief  Check whether a hostname is valid for the
    ///         wrapped certificate
    ///
    /// \param hostname  The hostname to verify
    ///
    /// \return  True if the hostname if valid for the wrapped
    ///          certificate, false otherwise
    bool isValid(const std::string& hostname);

  private:
    /// \brief  Check whether a hostname is valid for the subject
    ///         alternative names in the certificate
    ///
    /// Assumes that m_sanNames is not nullptr.
    ///
    /// \param hostname  The hostname to verify
    ///
    /// \return  True if the hostname if valid for the wrapped
    ///          certificate, false otherwise
    bool isSanValid(const std::string& hostname);

    /// \brief  Check whether a hostname is valid for the common
    ///         name in the certificate
    ///
    /// Assumes that m_commonName is not nullptr.
    ///
    /// \param hostname  The hostname to verify
    ///
    /// \return  True if the hostname if valid for the wrapped
    ///          certificate, false otherwise
    bool isCnValid(const std::string& hostname);

    /// \brief  Check whether a hostname is valid for a given name
    ///         in the certificate
    ///
    /// \param name      The name in the certificate
    /// \param hostname  The name to check against the certificate
    ///
    /// \return  True if the hostname matches, false otherwise
    bool isNameValid(ASN1_STRING* name, const std::string& hostname);

    /// The subject alternatives names in the certificate or nullptr
    /// if there aren't any
    stack_st_GENERAL_NAME* m_sanNames;
    /// The common name for the certificate if m_sanNames is nullptr
    X509_NAME_ENTRY* m_commonName;
};

}  // namespace openssl
}  // namespace dote
