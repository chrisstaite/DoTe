
#include "loop.h"
#include "log.h"

namespace dote {

void Loop::run()
{
    while (m_fds.size() > 0 &&
            poll(&m_fds.front(), m_fds.size(), timeout()) >= 0)
    {
        for (const auto& fd : m_fds)
        {
            if ((fd.revents & POLLIN))
            {
                auto func = m_readFunctions.find(fd.fd);
                if (func != m_readFunctions.end())
                {
                    func->second.first(fd.fd);
                }
            }
            if ((fd.revents & POLLOUT))
            {
                auto func = m_writeFunctions.find(fd.fd);
                if (func != m_writeFunctions.end())
                {
                    func->second.first(fd.fd);
                }
            }
            if ((fd.revents & ~(POLLIN | POLLOUT)))
            {
                (void) raiseException(fd.fd);
            }
        }
    }
}

bool Loop::registerRead(int handle, Callback callback, time_t timeout)
{
    if (m_readFunctions.count(handle))
    {
        return false;
    }

    registerFd(handle, POLLIN);
    m_readFunctions.insert({ handle, std::make_pair(std::move(callback), timeout) });
    return true;
}

bool Loop::registerWrite(int handle, Callback callback, time_t timeout)
{
    if (m_writeFunctions.count(handle))
    {
        return false;
    }

    registerFd(handle, POLLOUT);
    m_writeFunctions.insert({ handle, std::make_pair(std::move(callback), timeout) });
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
    m_exceptFunctions.insert({ handle, std::move(callback) });
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

int Loop::timeout()
{
    time_t now = time(nullptr);
    time_t earliest = 0u;

    for (auto it = m_readFunctions.begin(); it != m_readFunctions.end();)
    {
        time_t thisTime = it->second.second;
        if (thisTime != 0u && thisTime <= now && raiseException(it->first))
        {
            Log::info << "Timeout on read";
            it = m_readFunctions.erase(it);
        }
        else if (earliest == 0u || thisTime < earliest)
        {
            earliest = thisTime;
            ++it;
        }
        else
        {
            ++it;
        }
    }

    for (auto it = m_writeFunctions.begin(); it != m_writeFunctions.end();)
    {
        time_t thisTime = it->second.second;
        if (thisTime != 0u && thisTime <= now && raiseException(it->first))
        {
            Log::info << "Timeout on write";
            it = m_writeFunctions.erase(it);
        }
        else if (earliest == 0u || thisTime < earliest)
        {
            earliest = thisTime;
            ++it;
        }
        else
        {
            ++it;
        }
    }

    if (earliest == 0)
    {
        return -1;
    }
    else if (now >= earliest)
    {
        return 0;
    }
    return (earliest - now) * 1000u;
}

bool Loop::raiseException(int handle)
{
    auto func = m_exceptFunctions.find(handle);
    if (func != m_exceptFunctions.end())
    {
        func->second(handle);
        return true;
    }
    return false;
}

}  // namespace dote
