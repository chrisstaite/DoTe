
#include "forwarder_config.h"

namespace dote {

ForwarderConfig::ForwarderConfig() :
    m_forwarders()
{ }

void ForwarderConfig::addForwarder(const char *ip,
                                   const char *host,
                                   const char *pin,
                                   unsigned short port)
{
    Forwarder forwarder{
        std::string(ip), std::string(host), std::string(pin), port
    };
    m_forwarders.emplace_back(std::move(forwarder));
}

std::vector<ForwarderConfig::Forwarder>::const_iterator ForwarderConfig::get() const
{
    // TODO: Choose which is best rather than just the first based on a
    //       number of factors, i.e. whether there was a previous failure
    return m_forwarders.cbegin();
}

std::vector<ForwarderConfig::Forwarder>::const_iterator ForwarderConfig::end() const
{
    return m_forwarders.cend();
}

}  // namespace dote
