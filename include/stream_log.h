
#pragma once

#include <memory>
#include <sstream>

namespace dote {

/// \brief  A starter for logging which creates a new log
///         entry to log when it starts
class StreamLogStart
{
  public:
    /// \brief  Create a log stream that is able to create
    ///         logs
    ///
    /// \param level  The level to log for this stream
    StreamLogStart(int level);

    StreamLogStart(const StreamLogStart&) = delete;
    StreamLogStart& operator=(const StreamLogStart&) = delete;

    /// \brief  Get the level
    ///
    /// \return  The level of this logger
    int level() const;
    
  private:
    /// The level for this stream
    int m_level;
};

class StreamLog
{
  public:
    /// \brief  Construct a new instance for a given
    ///         start point
    StreamLog(StreamLogStart& start);

    StreamLog(const StreamLog&) = delete;
    StreamLog& operator=(const StreamLog&) = delete;
    StreamLog& operator=(StreamLog&& other) = delete;

    /// \brief  Move the log to this instance
    ///
    /// \param other  The instance to move from
    StreamLog(StreamLog&& other);

    /// \brief  Log the message
    ~StreamLog();

  private:
    template<typename T>
    friend StreamLog operator<<(StreamLog&& log, const T& value);

    /// The start stream
    StreamLogStart& m_start;
    /// The stream to build up the log message for
    std::unique_ptr<std::ostringstream> m_stream;
};

template<typename T>
StreamLog operator<<(StreamLog&& log, const T& value)
{
    if (log.m_stream)
    {
        *log.m_stream << value;
    }
    return std::move(log);
}

template<typename T>
StreamLog operator<<(StreamLogStart& start, const T& value)
{
    return StreamLog(start) << value;
}

}  // namespace dote
