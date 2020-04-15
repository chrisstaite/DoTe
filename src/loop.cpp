
#include "loop.h"
#include "log.h"

#include <algorithm>

namespace dote {

void Loop::run()
{
    int currentTimeout = timeout();
    while (m_fds.size() > 0 &&
            poll(&m_fds.front(), m_fds.size(), currentTimeout) >= 0)
    {
        // Take a copy of m_fds so it's not invalidated
        auto fds(m_fds);
        for (const auto& fd : fds)
        {
            if ((fd.revents & POLLIN))
            {
                callCallback(m_readFunctions, fd.fd);
            }
            if ((fd.revents & POLLOUT))
            {
                callCallback(m_writeFunctions, fd.fd);
            }
            if ((fd.revents & (POLLERR | POLLHUP | POLLNVAL)))
            {
                (void) raiseException(fd.fd);
            }
        }
        currentTimeout = timeout();
    }
}

void Loop::callCallback(
        const std::map<int, std::pair<Callback, time_t>>& functions,
        int handle)
{
    auto func = functions.find(handle);
    if (func != functions.end())
    {
        func->second.first(handle);
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

time_t Loop::timeout(time_t now, std::map<int, std::pair<Callback, time_t>>& functions)
{
    time_t earliest = 0u;
    std::vector<int> excepted;
    for (auto it = functions.begin(); it != functions.end();)
    {
        time_t thisTime = it->second.second;
        if (thisTime != 0u && thisTime <= now &&
                std::find(excepted.begin(), excepted.end(), it->first) == excepted.end() &&
                raiseException(it->first))
        {
            Log::info << "Timeout";
            // The iterator may have been invalidated by raiseException, so need to restart
            // Log the fact we've handled it, so we don't get in an infinite loop
            excepted.emplace_back(it->first);
            it = functions.begin();
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
    return earliest;
}

int Loop::timeout()
{
    time_t now = time(nullptr);
    time_t earliestRead = timeout(now, m_readFunctions);
    time_t earliestWrite = timeout(now, m_writeFunctions);
    time_t earliest = earliestRead < earliestWrite ? earliestRead : earliestWrite;
    if (earliest == 0u)
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
