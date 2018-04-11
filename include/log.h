
#pragma once

#include "i_logger.h"
#include "stream_log.h"

#ifdef NDEBUG
#include "noop_log.h"
#endif

#include <memory>

namespace dote {

/// \brief  The wrapper around all loggers
///
/// The logging is performed in a streaming manner,
/// i.e. Log::debug << "Hello";
class Log
{
  public:
    /// \brief  No instance of this, only static members
    Log() = delete;

#ifdef NDEBUG
    /// Compiled out debug logging on release
    static NoopLog debug;
#else
    /// Log at the debug level
    static StreamLogStart debug;
#endif
    /// Log at the info level
    static StreamLogStart info;
    /// Log at the notice level
    static StreamLogStart notice;
    /// Log at the warning level
    static StreamLogStart warn;
    /// Log at the error level
    static StreamLogStart err;
    /// Log at the critical level
    static StreamLogStart critical;

    /// \brief  Log a message to the registered logger
    ///
    /// \param level  The level to log at
    /// \param value  The value to log
    static void log(int level, const std::string& value);

    /// \brief  Set the logger
    ///
    /// \param logger  The logger to use
    static void setLogger(std::shared_ptr<ILogger> logger);

  private:
    /// The logger to log to
    static std::shared_ptr<ILogger> m_logger;
};

}  // namespace dote
