
#include "loop.h"

namespace dote {

void Loop::run()
{
    while (m_fds.size() > 0 &&
            poll(&m_fds.front(), m_fds.size(), -1) >= 0)
    {
        for (const auto& fd : m_fds)
        {
            if ((fd.revents & POLLIN))
            {
                auto func = m_readFunctions.find(fd.fd);
                if (func != m_readFunctions.end())
                {
                    func->second(fd.fd);
                }
            }
            if ((fd.revents & POLLOUT))
            {
                auto func = m_writeFunctions.find(fd.fd);
                if (func != m_writeFunctions.end())
                {
                    func->second(fd.fd);
                }
            }
            if ((fd.revents & ~(POLLIN | POLLOUT)))
            {
                auto func = m_exceptFunctions.find(fd.fd);
                if (func != m_exceptFunctions.end())
                {
                    func->second(fd.fd);
                }
            }
        }
    }
}

bool Loop::registerRead(int handle, Callback callback)
{
    if (m_readFunctions.count(handle))
    {
        return false;
    }

    registerFd(handle, POLLIN);
    m_readFunctions.insert( { handle, std::move(callback) } );
    return true;
}

bool Loop::registerWrite(int handle, Callback callback)
{
    if (m_writeFunctions.count(handle))
    {
        return false;
    }
    
    registerFd(handle, POLLOUT);
    m_writeFunctions.insert( { handle, std::move(callback) } );
    return true;
}

bool Loop::registerException(int handle, Callback callback)
{
    if (m_exceptFunctions.count(handle))
    {
        return false;
    }
    
    // Nothing to register for, poll always returns exceptions
    registerFd(handle, 0);
    m_exceptFunctions.insert( { handle, std::move(callback) } );
    return true;
}

void Loop::registerFd(int handle, short event)
{
    bool registered = false;
    for (auto& fd : m_fds)
    {
        if (fd.fd == handle)
        {
            fd.events |= event;
            registered = true;
        }
    }

    if (!registered)
    {
        pollfd fd { handle, event, 0 };
        m_fds.emplace_back(fd);
    }
}

void Loop::deregisterFd(int handle, short event)
{
    for (auto& fd : m_fds)
    {
        if (fd.fd == handle)
        {
            fd.events &= ~event;
        }
    }
}

void Loop::removeRead(int handle)
{
    m_readFunctions.erase(handle);
    deregisterFd(handle, POLLIN);
    cleanFd(handle);
}

void Loop::removeWrite(int handle)
{
    m_writeFunctions.erase(handle);
    deregisterFd(handle, POLLOUT);
    cleanFd(handle);
}

void Loop::removeException(int handle)
{
    m_exceptFunctions.erase(handle);
    cleanFd(handle);
}

void Loop::cleanFd(int handle)
{
    if (m_readFunctions.count(handle) == 0 &&
            m_writeFunctions.count(handle) == 0 &&
            m_exceptFunctions.count(handle) == 0)
    {
        auto it = m_fds.begin();
        while (it != m_fds.end())
        {
            if (it->fd == handle)
            {
                it = m_fds.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}

}  // namespace dote
