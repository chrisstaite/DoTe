
#pragma once

#include "i_logger.h"

#include <gmock/gmock.h>

namespace dote {

class MockLogger : public ILogger
{
  public:
    ~MockLogger() noexcept
    { }

    MOCK_METHOD2(log, void(int, const std::string&));
};

}  // namespace dote
