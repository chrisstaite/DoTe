
#pragma once

#include "i_loop.h"

#include <gmock/gmock.h>

namespace dote {

class MockLoop : public ILoop
{
  public:
    ~MockLoop() noexcept = default;

    MOCK_METHOD2(registerRead, bool(int, Callback));
    MOCK_METHOD2(registerWrite, bool(int, Callback));
    MOCK_METHOD2(registerException, bool(int, Callback));
    MOCK_METHOD1(removeRead, void(int));
    MOCK_METHOD1(removeWrite, void(int));
    MOCK_METHOD1(removeException, void(int));
};

}  // namespace dote
