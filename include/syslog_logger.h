
#pragma once

#include "i_logger.h"

namespace dote {

/// \brief  A logger that outputs to the syslog, there should
///         only be once instance since it opens the log in
///         the constructor and closes it in the destructor
class SyslogLogger : public ILogger
{
  public:
    /// \brief  Open the syslog
    SyslogLogger();
    
    SyslogLogger(const SyslogLogger&) = delete;
    SyslogLogger& operator=(const SyslogLogger&) = delete;

    /// \brief  Close the syslog
    ~SyslogLogger();
    
    /// \brief  Log the item to the syslog
    ///
    /// \param level  The level to log at
    /// \param value  The value to log
    void log(int level, const std::string& value) override;
};

}  // namespace dote
