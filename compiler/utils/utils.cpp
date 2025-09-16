#include "utils.h"
#include "error.h"
#include <fstream>
#include <sstream>

namespace aym {

std::string readFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        aym::error("No se pudo abrir el archivo: " + path);
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace aym
