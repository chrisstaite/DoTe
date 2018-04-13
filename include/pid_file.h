
#pragma once

#include <string>

namespace dote {

/// \brief  A class to manage a PID file which will open and
///         lock it, then unlock and delete on exit
class PidFile
{
  public:
    /// \brief  Write a PID file and lock it
    ///
    /// \param filename  The name of the PID file or empty
    ///                  to not write one
    PidFile(const std::string& filename);

    PidFile(const PidFile&) = delete;
    PidFile& operator=(const PidFile&) = delete;

    /// \brief  Unlock and delete the PID file
    ~PidFile();

    /// \brief  Determine if the PID file was created and
    ///         locked
    ///
    /// \return  True if the PID file was created and locked
    ///          or if the file name was empty, false otherwise
    bool valid() const;

  private:
    /// The name of the PID file to delete it
    std::string m_filename;
    /// The file descriptor for the pid file
    int m_handle;
};

}  // namespace dote
