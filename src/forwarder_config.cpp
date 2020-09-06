
#include "forwarder_config.h"

#include <algorithm>
#include <cstring>

namespace dote {

ForwarderConfig::ForwarderConfig() :
    m_timeout(5),
    m_forwarders()
{ }

void ForwarderConfig::clear()
{
    m_forwarders.clear();
}

void ForwarderConfig::addForwarder(const ConfigParser::Forwarder& config)
{
    m_forwarders.push_back(config);
}

std::vector<ConfigParser::Forwarder>::const_iterator ForwarderConfig::get() const
{
    return m_forwarders.cbegin();
}

std::vector<ConfigParser::Forwarder>::const_iterator ForwarderConfig::end() const
{
    return m_forwarders.cend();
}

void ForwarderConfig::setBad(const ConfigParser::Forwarder &config)
{
    // Look for the config in the forwarder list and move it to the end
    for (auto it = m_forwarders.begin(); it != m_forwarders.end(); ++it)
    {
        if (memcmp(&it->remote, &config.remote, sizeof(config.remote)) == 0)
        {
            std::rotate(it, it + 1, m_forwarders.end());
            break;
        }
    }
}

void ForwarderConfig::setTimeout(unsigned int timeout)
{
    m_timeout = timeout;
}

unsigned int ForwarderConfig::timeout() const
{
    return m_timeout;
}

}  // namespace dote
