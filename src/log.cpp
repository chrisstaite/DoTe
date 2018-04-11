
#include "log.h"

#include <syslog.h>

namespace dote {

#ifdef NDEBUG
NoopLog Log::debug;
#else
StreamLogStart Log::debug(LOG_DEBUG);
#endif
StreamLogStart Log::info(LOG_INFO);
StreamLogStart Log::notice(LOG_NOTICE);
StreamLogStart Log::warn(LOG_WARNING);
StreamLogStart Log::err(LOG_ERR);
StreamLogStart Log::critical(LOG_CRIT);
std::shared_ptr<ILogger> Log::m_logger;

void Log::log(int level, const std::string& value)
{
    if (m_logger)
    {
        m_logger->log(level, value);
    }
}

void Log::setLogger(std::shared_ptr<ILogger> logger)
{
    m_logger = std::move(logger);
}

}  // namespace dote
