
#pragma once

#include "i_forwarder_config.h"

#include <gmock/gmock.h>

namespace dote {

class MockForwarderConfig : public IForwarderConfig
{
  public:
    ~MockForwarderConfig() noexcept
    { }

    MOCK_METHOD1(addForwarder, void(const ConfigParser::Forwarder&));
    MOCK_METHOD1(setBad, void(const ConfigParser::Forwarder&));
    MOCK_CONST_METHOD0(get, std::vector<ConfigParser::Forwarder>::const_iterator());
    MOCK_CONST_METHOD0(end, std::vector<ConfigParser::Forwarder>::const_iterator());
    MOCK_CONST_METHOD0(timeout, unsigned int());
};

}  // namespace dote
