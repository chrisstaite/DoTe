
#pragma once

#include <functional>
#include <ctime>

namespace dote {

/// \brief  An interface to provide and event loop for handles
class ILoop
{
  public:
    virtual ~ILoop() = default;

    /// \brief  A registration type.
    enum Type {
      /// A special value to avoid deregistering on move.
      Moved,
      /// A read registration.
      Read,
      /// A write registration.
      Write,
      /// An exception registration.
      Exception,
    };

    /// \brief  This class is created with a read, write or exception
    ///         registration and automatically removes the registration
    ///         on destruction.
    ///
    /// NOTE: This object must not outlive the Loop that created it.
    class Registration
    {
      public:
        Registration(const Registration&) = delete;
        Registration& operator=(const Registration&) = delete;

        /// \brief  Construct an invalid Registration.
        Registration();

        /// \brief  Create a registration for a given loop.
        ///
        /// \param loop    The loop that this registration is for.
        /// \param handle  The handle of the registration
        /// \param type    The type of the registration.
        Registration(ILoop* loop, int handle, Type type);

        /// \brief  Take ownership of a registration.
        ///
        /// \param other  The instance to take ownership from.
        Registration(Registration&& other);

        /// \brief  Take ownership of a registration.
        ///
        /// \param other  The instance to take ownership from.
        Registration& operator=(Registration&& other);

        /// \brief  Deregister from the loop.
        ~Registration();

        operator bool() const { return valid(); }

        /// \brief  Whether the registration is valid (i.e. the execution was
        //          successful).
        bool valid() const;

        /// \brief  Clear the registration.
        void reset();

      private:
        friend class ILoop;

        /// The loop that this registration is for.
        ILoop* m_loop;
        /// The handle to deregister.
        int m_handle;
        /// The type of registration.
        Type m_type;
    };

    /// The type to call when the loop is available, called with the
    /// handle that caused it to be called
    using Callback = std::function<void(int)>;

    /// \brief   Register for read availability on a given handle
    ///
    /// \param handle    The handle to register for reading
    /// \param callback  The callback to call if it triggers
    /// \param timeout  The time at which to call exception on the handle
    ///
    /// \return  True if the handle is not already registered and now is
    virtual Registration registerRead(int handle, Callback callback, time_t timeout) = 0;

    /// \brief   Register for write availability on a given handle
    ///
    /// \param handle    The handle to register for writing
    /// \param callback  The callback to call if it triggers
    /// \param timeout  The time at which to call exception on the handle
    ///
    /// \return  True if the handle is not already registered and now is
    virtual Registration registerWrite(int handle, Callback callback, time_t timeout) = 0;

    /// \brief  Register for exceptions on a given handle
    ///
    /// \param handle    The handle to register for exceptions
    /// \param callback  The callback to call if it triggers
    ///
    /// \return  True if the handle is not already registered and now is
    virtual Registration registerException(int handle, Callback callback) = 0;

  protected:
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
