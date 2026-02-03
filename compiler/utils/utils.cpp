#include "utils.h"
#include "error.h"
#include "fs.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <climits>
#include <unistd.h>
#endif

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

std::string executableDir() {
    fs::path result;
#ifdef _WIN32
    std::vector<char> buffer(MAX_PATH);
    DWORD size = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size > 0) {
        result = fs::path(std::string(buffer.data(), size)).parent_path();
    }
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
        result = fs::path(buffer.data()).parent_path();
    }
#else
    std::vector<char> buffer(PATH_MAX);
    ssize_t len = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (len > 0) {
        buffer[len] = '\0';
        result = fs::path(buffer.data()).parent_path();
    }
#endif
    if (result.empty()) {
        result = fs::current_path();
    }
    return result.string();
}

} // namespace aym
