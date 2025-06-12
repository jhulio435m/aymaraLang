#include "utils.h"
#include <fstream>
#include <sstream>

namespace aym {

std::string readFile(const std::string &path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace aym
