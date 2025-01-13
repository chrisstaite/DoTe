
#include "loop.h"
#include "log.h"

#include <algorithm>

namespace dote {

void Loop::run()
{
    int currentTimeout = timeout();
    std::vector<pollfd> fds;
    populateFds(fds);
    while (fds.size() > 0 &&
            poll(&fds.front(), fds.size(), currentTimeout) >= 0)
    {
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
        populateFds(fds);
        currentTimeout = timeout();
    }
}

void Loop::populateFds(std::vector<pollfd>& fds)
{
    clearFds(fds);
    popluateFds(fds, m_readFunctions, POLLIN);
    popluateFds(fds, m_writeFunctions, POLLOUT);
    popluateFds(fds, m_exceptFunctions, POLLERR);
    removeFds(fds);
}

template<typename T>
void Loop::popluateFds(std::vector<pollfd>& fds,
                       const std::map<int, T>& functions,
                       short event)
{
    for (auto& handle_function : functions)
    {
        bool found = false;
        for (auto& fd : fds)
        {
            if (fd.fd == handle_function.first)
            {
                fd.events |= event;
                found = true;
            }
        }
        if (!found)
        {
            fds.emplace_back(pollfd { handle_function.first, event, 0 });
        }
    }
}

void Loop::clearFds(std::vector<pollfd> &fds)
{
    for (auto& fd : fds)
    {
        fd.events = 0;
    }
}

void Loop::removeFds(std::vector<pollfd> &fds)
{
    for (auto it = fds.begin(); it != fds.end();)
    {
        if (it->events == 0)
        {
            it = fds.erase(it);
        }
        else
        {
            it->events &= ~POLLERR;
            ++it;
        }
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

ILoop::Registration Loop::registerRead(int handle, Callback callback, time_t timeout)
{
    if (m_readFunctions.count(handle))
    {
        return {};
    }

    m_readFunctions.insert({ handle, std::make_pair(std::move(callback), timeout) });
    return Registration(this, handle, Type::Read);
}

ILoop::Registration Loop::registerWrite(int handle, Callback callback, time_t timeout)
{
    if (m_writeFunctions.count(handle))
    {
        return {};
    }

    m_writeFunctions.insert({ handle, std::make_pair(std::move(callback), timeout) });
    return Registration(this, handle, Type::Write);
}

ILoop::Registration Loop::registerException(int handle, Callback callback)
{
    if (m_exceptFunctions.count(handle))
    {
        return {};
    }

    // Nothing to register for, poll always returns exceptions
    m_exceptFunctions.insert({ handle, std::move(callback) });
    return Registration(this, handle, Type::Exception);
}

void Loop::removeRead(int handle)
{
    m_readFunctions.erase(handle);
}

void Loop::removeWrite(int handle)
{
    m_writeFunctions.erase(handle);
}

void Loop::removeException(int handle)
{
    m_exceptFunctions.erase(handle);
}

time_t Loop::timeout(time_t now, std::map<int, std::pair<Callback, time_t>>& functions)
{
    time_t earliest = 0u;
    std::vector<int> excepted;
    for (auto it = functions.begin(); it != functions.end();)
    {
        time_t thisTime = it->second.second;
        // Cache the handle so we can reference it after calling raiseException
        int handle = it->first;
        if (thisTime != 0u && thisTime <= now &&
                std::find(excepted.begin(), excepted.end(), handle) == excepted.end() &&
                raiseException(handle))
        {
            Log::info << "Timeout";
            // The iterator may have been invalidated by raiseException, so need to restart
            // Log the fact we've handled it, so we don't get in an infinite loop
            excepted.emplace_back(handle);
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
