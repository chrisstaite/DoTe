
#pragma once

#include "i_logger.h"

namespace dote {

/// \brief  A logger that outputs to the console
class ConsoleLogger : public ILogger
{
  public:
    ConsoleLogger() = default;

    ConsoleLogger(const ConsoleLogger&) = delete;
    ConsoleLogger& operator=(const ConsoleLogger&) = delete;
    
    /// \brief  Log the item to the console
    ///
    /// \param level  The level to log at
    /// \param value  The value to log
    void log(int level, const std::string& value) override;
};

}  // namespace dote
