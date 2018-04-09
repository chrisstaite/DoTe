
#include "forwarder_config.h"
#include "openssl/base64.h"

namespace dote {

ForwarderConfig::ForwarderConfig() :
    m_forwarders()
{ }

bool ForwarderConfig::addForwarder(const char *ip,
                                   const char *host,
                                   const char *pin,
                                   unsigned short port)
{
    std::vector<unsigned char> decodedPin(openssl::Base64::decode(pin));
    if (*pin && decodedPin.empty())
    {
        return false;
    }

    Forwarder forwarder{
        std::string(ip), std::string(host), std::move(decodedPin), port
    };
    m_forwarders.emplace_back(std::move(forwarder));
    return true;
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
