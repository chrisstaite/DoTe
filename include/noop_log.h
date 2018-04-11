
#pragma once

namespace dote {

/// \brief  A logger that doesn't do anything
class NoopLog
{ };

/// \brief  A streaming implementation that does nothing
///         for this logger type
///
/// \tparam T  The type of the thing to stream
///
/// \param log  The log instance to pass on
///
/// \return  A reference to the logger that was passed in
template<typename T>
NoopLog& operator<<(NoopLog& log, const T&)
{
    return log;
}

}  // namespace dote
