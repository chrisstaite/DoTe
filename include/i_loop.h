
#pragma once

#include <functional>

namespace dote {

/// \brief  An interface to provide and event loop for handles
class ILoop
{
  public:
    virtual ~ILoop() = default;

    /// The type to call when the loop is available, called with the
    /// handle that caused it to be called
    using Callback = std::function<void(int)>;

    /// \brief   Register for read availability on a given handle
    ///
    /// \param handle    The handle to register for reading
    /// \param callback  The callback to call if it triggers
    ///
    /// \return  True if the handle is not already registered and now is
    virtual bool registerRead(int handle, Callback callback) = 0;

    /// \brief   Register for write availability on a given handle
    ///
    /// \param handle    The handle to register for writing
    /// \param callback  The callback to call if it triggers
    ///
    /// \return  True if the handle is not already registered and now is
    virtual bool registerWrite(int handle, Callback callback) = 0;

    /// \brief  Register for exceptions on a given handle
    ///
    /// \param handle    The handle to register for exceptions
    /// \param callback  The callback to call if it triggers
    ///
    /// \return  True if the handle is not already registered and now is
    virtual bool registerException(int handle, Callback callback) = 0;

    /// \brief  Remove a read handle from the loop
    ///
    /// \param handle  The handle to remove read handles for
    virtual void removeRead(int handle) = 0;

    /// \brief  Remove a write handle from the loop
    ///
    /// \param handle  The handle to remove write handles for
    virtual void removeWrite(int handle) = 0;

    /// \brief  Remove a exception handle from the loop
    ///
    /// \param handle  The handle to remove exception handles for
    virtual void removeException(int handle) = 0;
};

}  // namespace dote
