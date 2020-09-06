
#pragma once

#include <string>
#include <vector>

namespace dote {

class ConfigParser;

/// \brief  A class to load the configuration from VyOS if available
class Vyatta
{
  public:
    /// \brief  Load the VyOS library and load if possible
    Vyatta();

    /// \brief  Destroy configuration object and close library
    ~Vyatta();

    /// \brief  Load the configuration from VyOS into the config parser
    ///
    /// \param parser  The configuration parser to load the configuration into
    void loadConfig(ConfigParser& parser);

  private:
    /// \brief  Get a configuration value for the given configuration path
    ///
    /// \param path  The configuration path to get the value for
    ///
    /// \return  The value at the path or the empty string if not found
    std::string getValue(const std::vector<std::string>& path);

    /// \brief  Load the configured servers into the config parser
    ///
    /// \param parser  The configuration parser to load the configuration into
    void loadServers(ConfigParser& parser);

    /// \brief  Load the configured forwarders into the config parser
    ///
    /// \param parser  The configuration parser to load the configuration into
    void loadForwarders(ConfigParser& parser);
    
    /// The handle to the VyOS library
    void* m_handle;
    /// The symbol to free the config
    void* m_free;
    /// The symbol to see if a configuration item exists
    void* m_exists;
    /// The symbol to get the configuration
    void* m_get;
    /// The handle to the configuration
    void* m_config;
};

}  // namespace dote
