
#pragma once

#include <string>

namespace dote {

/// \brief  The interface which is used to log any message at
///         any level
class ILogger
{
  public:
    virtual ~ILogger() = default;

    /// \brief  Log a message at a given level
    ///
    /// \param level  The level to log at
    /// \param value  The value to log
    virtual void log(int level, const std::string& value) = 0;
};

}  // namespace dote
