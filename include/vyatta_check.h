
#pragma once

#include "config_parser.h"

namespace dote {

class Dote;

/// \brief  A class to make a file handle that is made available upon configuration change
class VyattaCheck
{
  public:
    /// \brief  Start monitoring the configuration for changes
    ///
    /// \param parser  The base command line arguments to augment with Vyatta configuration
    explicit VyattaCheck(const ConfigParser& parser);

    /// \brief  Stop monitoring the configuration
    ~VyattaCheck();

    VyattaCheck(const VyattaCheck&) = delete;
    VyattaCheck& operator=(const VyattaCheck&) = delete;

    /// \brief  Load the working configuration over the base configuration
    ///
    /// \return  The new working configuration
    ConfigParser workingConfig() const;

    /// \brief  Configure the Dote instance to monitor the configuration changes
    ///
    /// \param dote  The Dote instance to use to monitor for changes
    void configure(Dote& dote);
    
  private:
    /// \brief  Handle a read event on the inotify handle
    void handleRead(int);
    
    /// \brief  Reload the configuration
    void reloadConfiguration();
    
    /// A handle to the inotify watch on the configuration file
    int m_fd;
    /// The main DoTe instance to configure on configuration changes
    Dote* m_dote;
    /// The base configuration to augment
    ConfigParser m_config;
};

}  // namespace dote
