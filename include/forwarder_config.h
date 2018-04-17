
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

  private:
    /// The available forwarders that can be opened
    std::vector<ConfigParser::Forwarder> m_forwarders;
};

}  // namespace dote
