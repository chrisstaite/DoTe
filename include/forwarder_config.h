
#pragma once

#include "i_forwarder_config.h"

#include <string>
#include <vector>

namespace dote {

/// \brief  An encapsulation around the configurations
class ForwarderConfig : public IForwarderConfig
{
  public:
    /// \brief  Create an empty configuration set
    ForwarderConfig();

    ForwarderConfig(const ForwarderConfig&) = delete;
    ForwarderConfig& operator=(const ForwarderConfig&) = delete;

    /// \brief  Has to be noexcept as override
    ~ForwarderConfig() noexcept = default;

    /// \brief  Add a DNS server to forward to
    ///
    /// \param config  The configuration to add
    void addForwarder(const ConfigParser::Forwarder& config) override;

    /// \brief  Notify of a failed connection to a given forwarder
    ///
    /// \param config  The forwarder that the connection failed for
    void setBad(const ConfigParser::Forwarder& config) override;

    /// \brief  Get the configuration to use, must check against
    ///         end() before using it
    ///
    /// \return  The chosen configuration
    std::vector<ConfigParser::Forwarder>::const_iterator get() const override;

    /// \brief  Get the end marker for the configuration
    ///
    /// \return  The invalid configuration marker
    std::vector<ConfigParser::Forwarder>::const_iterator end() const override;

    /// \brief  Set the number of seconds a forwarder has to respond
    ///
    /// \param timeout  The number of seconds to wait for a forwarder
    void setTimeout(unsigned int timeout);

    /// \brief  Get the number of seconds to wait until giving up on a connection
    ///
    /// \return  The number of seconds to have a connection open for
    unsigned int timeout() const override;

  private:
    /// The number of seconds to have a connection open for
    unsigned int m_timeout;
    /// The available forwarders that can be opened
    std::vector<ConfigParser::Forwarder> m_forwarders;
};

}  // namespace dote
