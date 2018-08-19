
#include "console_logger.h"

#include <iostream>

namespace dote {

void ConsoleLogger::log(int level, const std::string& value)
{
    std::cerr << value << std::endl;
}

}  // namespace dote
