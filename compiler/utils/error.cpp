#include "error.h"
#include <iostream>

namespace aym {

void error(const std::string &msg) {
    std::cerr << "[aymc] " << msg << std::endl;
}

} // namespace aym
