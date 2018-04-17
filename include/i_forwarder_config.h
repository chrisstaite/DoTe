
#pragma once

#include "config_parser.h"

#include <vector>

namespace dote {

/// \brief  An interface that manages forwarder configurations
class IForwarderConfig
{
  public:
    virtual ~IForwarderConfig() noexcept = default;

    /// \brief  Add a DNS server to forward to
    ///
    /// \param config  The configuration to add
    virtual void addForwarder(const ConfigParser::Forwarder& config) = 0;

    /// \brief  Notify of a failed connection to a given forwarder
    ///
    /// \param config  The forwarder that the connection failed for
    virtual void setBad(const ConfigParser::Forwarder& config) = 0;

    /// \brief  Get the configuration to use, must check against
    ///         end() before using it
    ///
    /// \return  The chosen configuration
    virtual std::vector<ConfigParser::Forwarder>::const_iterator get() const = 0;

    /// \brief  Get the end marker for the configuration
    ///
    /// \return  The invalid configuration marker
    virtual std::vector<ConfigParser::Forwarder>::const_iterator end() const = 0;
};

}  // namespace dote
