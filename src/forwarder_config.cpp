
#include "forwarder_config.h"

namespace dote {

ForwarderConfig::ForwarderConfig() :
    m_forwarders()
{ }

void ForwarderConfig::addForwarder(const ConfigParser::Forwarder& config)
{
    m_forwarders.push_back(config);
}

std::vector<ConfigParser::Forwarder>::const_iterator ForwarderConfig::get() const
{
    // TODO: Choose which is best rather than just the first based on a
    //       number of factors, i.e. whether there was a previous failure
    return m_forwarders.cbegin();
}

std::vector<ConfigParser::Forwarder>::const_iterator ForwarderConfig::end() const
{
    return m_forwarders.cend();
}

}  // namespace dote
