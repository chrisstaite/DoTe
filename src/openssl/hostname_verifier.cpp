
#include "openssl/hostname_verifier.h"

#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/asn1.h>

#include <cstring>

namespace dote {
namespace openssl {

namespace {

/// \brief  Convert a char to lowercase if it is ASCII
///
/// \param c  The char to convert
///
/// \return  The converted char
char lower(char c)
{
    if (c > 'A' && c < 'Z')
    {
        return c + ('a' - 'A');
    }
    return c;
}

/// \brief  Determine whether two characters match with DNS
///         case insensitivity for ASCII characters
///
/// \param a  The first char to compare
/// \param b  The second char to compare
///
/// \return  True if there is a case-insensitive match
bool matchChar(char a, char b)
{
    // Only support ASCII
    return lower(a) == lower(b);
}

/// \brief  Check the hostname matches the certificate name
///         including wildcards.
///
/// The hostname matches if it completely matches, or if it
/// matches the end of the certificate and the previous in
/// the hostname is either nothing or a . and the previous
/// in the certificate is *. where there is at least two .'s
/// in the certificateName.
///
/// \param hostname         The hostname to check if it is in
///                         the certificate
/// \param certificateName  The hostname with optional wildcard
///                         in the certificate, must not contain
///                         a null value
/// \param length           The length of certificateName
///
/// \return  True if the hostname matches, false otherwise
bool checkHostname(const std::string& hostname,
                   const char* certificateName,
                   size_t length)
{
    // If there are any null bytes in the string we disallow
    if (memchr(certificateName, 0, length) != nullptr)
    {
        return false;
    }

    // If the certificate name is blank then it doesn't match
    if (length == 0)
    {
        return false;
    }

    // Reverse match the hostname against the certificate
    size_t certLength = length;
    size_t hostLength = hostname.length();
    size_t dotCount = 0;
    while (certLength > 0 && hostLength > 0)
    {
        --certLength;
        --hostLength;
        if (!matchChar(
                hostname.at(hostLength), certificateName[certLength]
            ))
        {
            // No match at this point, verify if it's wildcard or end
            break;
        }
        if (certificateName[certLength] == '.')
        {
            ++dotCount;
        }
    }

    // Check if it was a full string match
    if (certLength == 0 && hostLength == 0)
    {
        return true;
    }

    // If the certificate is a * and the length char is a '.' then
    // its a wildcard match
    if (certificateName[certLength] == '*' &&
            certificateName[certLength + 1] == '.')
    {
        // Only match if we have more than two '.'
        return dotCount >= 2;
    }

    // No match
    return false;
}

}  // anon namespace

HostnameVerifier::HostnameVerifier(X509* certificate) :
    m_sanNames(nullptr),
    m_commonName(nullptr)
{
    m_sanNames = reinterpret_cast<stack_st_GENERAL_NAME*>(
        X509_get_ext_d2i(
            certificate, NID_subject_alt_name, nullptr, nullptr
        )
    );
    if (!m_sanNames)
    {
        int index = X509_NAME_get_index_by_NID(
            X509_get_subject_name(certificate), NID_commonName, -1
        );
        if (index >= 0)
        {
            m_commonName = X509_NAME_get_entry(
                X509_get_subject_name(certificate), index
            );
        }
    }
}

HostnameVerifier::~HostnameVerifier()
{
    if (m_sanNames)
    {
        sk_GENERAL_NAME_pop_free(m_sanNames, GENERAL_NAME_free);
    }
}

bool HostnameVerifier::isNameValid(ASN1_STRING* name,
                                   const std::string &hostname)
{
    const char* utfName =
        reinterpret_cast<char*>(ASN1_STRING_data(name));
    size_t length = ASN1_STRING_length(name);
    return checkHostname(hostname, utfName, length);
}

bool HostnameVerifier::isSanValid(const std::string& hostname)
{
    for (int i = 0; i < sk_GENERAL_NAME_num(m_sanNames); ++i)
    {
        const GENERAL_NAME *currentName =
            sk_GENERAL_NAME_value(m_sanNames, i);
        if (currentName->type == GEN_DNS)
        {
            if (isNameValid(currentName->d.dNSName, hostname))
            {
                return true;
            }
        }
    }
    return false;
}

bool HostnameVerifier::isCnValid(const std::string& hostname)
{
    ASN1_STRING* commonName =
        X509_NAME_ENTRY_get_data(m_commonName);
    if (commonName)
    {
        return isNameValid(commonName, hostname);
    }
    return false;
}

bool HostnameVerifier::isValid(const std::string& hostname)
{
    if (hostname.empty())
    {
        // Empty hostname means no validation
        return true;
    }
    else if (m_sanNames)
    {
        return isSanValid(hostname);
    }
    else if (m_commonName)
    {
        // Don't check CN if SAN set
        return isCnValid(hostname);
    }
    return false;
}

}  // namespace dote
}  // namespace openssl
