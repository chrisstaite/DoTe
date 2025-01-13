
#include "pid_file.h"
#include "log.h"

#include <fcntl.h>
#include <unistd.h>

namespace dote {

PidFile::PidFile(const std::string& filename) :
    m_filename(filename),
    m_handle(-1)
{
    pid_t pid = getpid();

    if (pid != -1 && !m_filename.empty())
    {
        m_handle = open(filename.c_str(), O_RDWR | O_CREAT, 0640);
        if (m_handle < 0)
        {
            Log::err << "Unable to open PID file";
        }
        else if (lockf(m_handle, F_TLOCK, 0) < 0)
        {
            Log::err << "Unable to lock PID file";
            close(m_handle);
            m_handle = -1;
        }
    }

    if (m_handle >= 0)
    {
        std::string contents = std::to_string(pid) + "\n";
        if (write(m_handle, contents.c_str(), contents.length()) !=
                contents.length())
        {
            Log::err << "Unable to write the PID to the PID file";
            (void) close(m_handle);
            m_handle = -1;
            (void) unlink(m_filename.c_str());
        }
    }
}

PidFile::~PidFile()
{
    if (m_handle >= 0)
    {
        (void) lockf(m_handle, F_ULOCK, 0);
        (void) close(m_handle);
        (void) unlink(m_filename.c_str());
    }
}

bool PidFile::valid() const
{
    return m_filename.empty() || m_handle != -1;
}

}  // namespace dote
