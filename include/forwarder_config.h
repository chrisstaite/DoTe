
#pragma once

#include <string>
#include <vector>

namespace dote {

/// \brief  An encapsulation around the configurations
class ForwarderConfig
{
  public:
    /// \brief  All the details required to connect to a forwarder
    struct Forwarder
    {
        /// The IP to connect to and forward to
        std::string ip;
        /// The host to verify the certificate common name against
        std::string host;
        /// The base64 encoded SHA-256 hash of the certificate
        std::vector<unsigned char> pin;
        /// The port to connect to
        unsigned short port;
    };

    /// \brief  Create an empty configuration set
    ForwarderConfig();

    ForwarderConfig(const ForwarderConfig&) = delete;
    ForwarderConfig& operator=(const ForwarderConfig&) = delete;

    /// \brief  Add a DNS server to forward to
    ///
    /// \param ip    The IP address to connect to and forward
    ///              connections to
    /// \param host  The expected host for the TLS connection
    /// \param pin   The base64 encoded SHA-256 hash of the
    ///              certificate that is connected to
    /// \param port  The port to connect to
    ///
    /// \return  True if the forwarder is valid or false if not
    bool addForwarder(const char* ip,
                      const char* host,
                      const char* pin,
                      unsigned short port = 853);

    /// \brief  Get the configuration to use, must check against
    ///         end() before using it
    ///
    /// \return  The chosen configuration
    std::vector<Forwarder>::const_iterator get() const;

    /// \brief  Get the end marker for the configuration
    ///
    /// \return  The invalid configuration marker
    std::vector<Forwarder>::const_iterator end() const;

  private:
    /// The available forwarders that can be opened
    std::vector<Forwarder> m_forwarders;
};

}  // namespace dote
