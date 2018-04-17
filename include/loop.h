
#pragma once

#include "i_loop.h"

#include <poll.h>

#include <vector>
#include <map>

namespace dote {

/// \brief  An event loop that does polling
class Loop : public ILoop
{
  public:
    /// \brief  Create the looper
    Loop() = default;

    Loop(const Loop&) = delete;
    Loop& operator=(const Loop&) = delete;

    /// \brief  Has to be noexcept as override
    ~Loop() noexcept = default;

    /// \brief  Run the loop
    void run();

    /// \brief   Register for read availability on a given handle
    ///
    /// \param handle    The handle to register for reading
    /// \param callback  The callback to call if it triggers
    ///
    /// \return  True if the handle is not already registered and now is
    bool registerRead(int handle, Callback callback) override;

    /// \brief   Register for write availability on a given handle
    ///
    /// \param handle    The handle to register for writing
    /// \param callback  The callback to call if it triggers
    ///
    /// \return  True if the handle is not already registered and now is
    bool registerWrite(int handle, ILoop::Callback callback) override;

    /// \brief  Register for exceptions on a given handle
    ///
    /// \param handle    The handle to register for exceptions
    /// \param callback  The callback to call if it triggers
    ///
    /// \return  True if the handle is not already registered and now is
    bool registerException(int handle, ILoop::Callback callback) override;

    /// \brief  Remove a read handle from the loop
    ///
    /// \param handle  The handle to remove read handles for
    void removeRead(int handle) override;

    /// \brief  Remove a write handle from the loop
    ///
    /// \param handle  The handle to remove write handles for
    void removeWrite(int handle) override;

    /// \brief  Remove a exception handle from the loop
    ///
    /// \param handle  The handle to remove exception handles for
    void removeException(int handle) override;

  private:
    /// \brief  Register a given handle with m_fds for the poll
    ///
    /// \param handle  The handle to register for events for
    /// \param event   The event type to register for
    void registerFd(int handle, short event);

    /// \brief  Deregister a given handle with m_fds for the poll
    ///
    /// \param handle  The handle to deregister for events for
    /// \param event   The event type to deregister for
    void deregisterFd(int handle, short event);

    /// \brief  If the handle doesn't exist in m_readFunctions,
    ///         m_writeFunctions or m_exceptFunctions then remove
    ///         it from m_fds.
    ///
    /// \param handle  The handle to check and remove
    void cleanFd(int handle);

    /// The read handles
    std::map<int, Callback> m_readFunctions;
    /// The write handles
    std::map<int, Callback> m_writeFunctions;
    /// The exception handles
    std::map<int, Callback> m_exceptFunctions;
    /// The contraction of all the callback handles for use with poll
    std::vector<pollfd> m_fds;
};

}  // namespace dote
