
#include "log.h"
#include "stream_log.h"

namespace dote {

StreamLogStart::StreamLogStart(int level) :
    m_level(level)
{ }

int StreamLogStart::level() const
{
    return m_level;
}

StreamLog::StreamLog(StreamLogStart& start) :
    m_start(start),
    m_stream(new ::std::ostringstream())
{ }

StreamLog::StreamLog(StreamLog&& other) :
    m_start(other.m_start),
    m_stream(std::move(other.m_stream))
{ }

StreamLog::~StreamLog()
{
    if (m_stream)
    {
        Log::log(m_start.level(), m_stream->str());
    }
}

}  // namespace dote
