
#include "syslog_logger.h"

#include <syslog.h>

namespace dote {

SyslogLogger::SyslogLogger()
{
    openlog("DoTe", LOG_CONS | LOG_PID, LOG_DAEMON);
}

SyslogLogger::~SyslogLogger() noexcept
{
    closelog();
}

void SyslogLogger::log(int level, const std::string& value)
{
    syslog(level, "%s", value.c_str());
}

}  // namespace dote
