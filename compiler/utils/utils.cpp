#include "utils.h"
#include "error.h"
#include "fs.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <climits>
#include <unistd.h>
#endif

namespace aym {

namespace {

std::string getEnvVarImpl(const std::string &name) {
    const char *value = std::getenv(name.c_str());
    return value ? std::string(value) : "";
}

std::string toUpperAscii(std::string value) {
    for (char &ch : value) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string toolFileName(const std::string &toolName) {
#ifdef _WIN32
    if (toolName.size() >= 4) {
        const std::string suffix = toolName.substr(toolName.size() - 4);
        if (suffix == ".exe" || suffix == ".EXE") {
            return toolName;
        }
    }
    return toolName + ".exe";
#else
    return toolName;
#endif
}

void addUniqueCandidate(std::vector<fs::path> &candidates, const fs::path &candidate) {
    if (candidate.empty()) {
        return;
    }
    const fs::path normalized = candidate.lexically_normal();
    for (const auto &existing : candidates) {
        if (existing.lexically_normal() == normalized) {
            return;
        }
    }
    candidates.push_back(normalized);
}

void addToolCandidates(std::vector<fs::path> &candidates,
                       const fs::path &root,
                       const std::string &toolName,
                       const std::string &fileName) {
    const bool prefersMingwBin =
        toolName == "gcc" || toolName == "g++" || toolName == "ld" || toolName == "as";

    if (prefersMingwBin) {
        addUniqueCandidate(candidates, root / "toolchain" / "mingw64" / "bin" / fileName);
        addUniqueCandidate(candidates, root / "mingw64" / "bin" / fileName);
    }

    addUniqueCandidate(candidates, root / "toolchain" / "bin" / fileName);
    addUniqueCandidate(candidates, root / "bin" / fileName);

    if (!prefersMingwBin) {
        addUniqueCandidate(candidates, root / "toolchain" / "mingw64" / "bin" / fileName);
        addUniqueCandidate(candidates, root / "mingw64" / "bin" / fileName);
    }
}

} // namespace

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

fs::path executablePath() {
    fs::path result;
#ifdef _WIN32
    std::vector<char> buffer(MAX_PATH);
    DWORD size = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size > 0) {
        result = fs::path(std::string(buffer.data(), size));
    }
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
        result = fs::path(buffer.data());
    }
#else
    std::vector<char> buffer(PATH_MAX);
    ssize_t len = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (len > 0) {
        buffer[len] = '\0';
        result = fs::path(buffer.data());
    }
#endif
    if (result.empty()) {
        result = fs::current_path() / toolFileName("aymc");
    }
    return result.lexically_normal();
}

fs::path executableDirPath() {
    return executablePath().parent_path().lexically_normal();
}

std::string executableDir() {
    return executableDirPath().string();
}

std::string getEnvVar(const std::string &name) {
    return getEnvVarImpl(name);
}

bool setEnvVar(const std::string &name, const std::string &value) {
#ifdef _WIN32
    // Use SetEnvironmentVariableA so CreateProcess (which we use in process.cpp) sees it.
    // Also use _putenv_s to keep the CRT environment in sync.
    _putenv_s(name.c_str(), value.c_str());
    return SetEnvironmentVariableA(name.c_str(), value.c_str()) != 0;
#else
    return setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
}

void prependToPath(const std::string &directory) {
    if (directory.empty()) return;
    std::string currentPath = getEnvVarImpl("PATH");
    char sep = ';';
#ifndef _WIN32
    sep = ':';
#endif
    if (currentPath.empty()) {
        setEnvVar("PATH", directory);
    } else {
        setEnvVar("PATH", directory + sep + currentPath);
    }
}

fs::path findBundledToolExecutable(const std::string &toolName) {
    if (toolName.empty()) {
        return fs::path();
    }

    const std::string specificOverride = getEnvVarImpl("AYM_" + toUpperAscii(toolName) + "_PATH");
    if (!specificOverride.empty()) {
        std::error_code ec;
        const fs::path overridePath = fs::absolute(fs::path(specificOverride), ec).lexically_normal();
        if (!ec && fs::exists(overridePath)) {
            return overridePath;
        }
    }

    const std::string fileName = toolFileName(toolName);
    std::vector<fs::path> roots;

    const std::string toolchainRootEnv = getEnvVarImpl("AYM_TOOLCHAIN_ROOT");
    if (!toolchainRootEnv.empty()) {
        addUniqueCandidate(roots, fs::path(toolchainRootEnv));
    }

    const fs::path exeDir = executableDirPath();
    addUniqueCandidate(roots, exeDir);
    if (exeDir.has_parent_path()) {
        addUniqueCandidate(roots, exeDir.parent_path());
    }

    std::vector<fs::path> candidates;
    for (const auto &root : roots) {
        addToolCandidates(candidates, root, toolName, fileName);
    }

    std::error_code ec;
    for (const auto &candidate : candidates) {
        ec.clear();
        if (fs::exists(candidate, ec) && !ec) {
            return fs::absolute(candidate, ec).lexically_normal();
        }
    }

    return fs::path();
}

std::string resolveToolExecutable(const std::string &toolName) {
    const fs::path bundled = findBundledToolExecutable(toolName);
    if (!bundled.empty()) {
#ifdef _WIN32
        // On Windows, bundled tools often need their bin directories in PATH 
        // to find internal components (cc1) and DLLs.
        // We prepend the immediate parent, and also try to find the toolchain root
        // to add the other common bin directory.
        fs::path toolDir = bundled.parent_path();
        prependToPath(toolDir.string());
        
        // Try to find the 'toolchain' folder in the path to add both bin and mingw64/bin
        fs::path p = toolDir;
        while (!p.empty() && p.has_parent_path()) {
            if (p.filename() == "toolchain") {
                prependToPath((p / "bin").string());
                prependToPath((p / "mingw64" / "bin").string());
                break;
            }
            p = p.parent_path();
        }
#endif
        return bundled.string();
    }
    return toolFileName(toolName);
}

} // namespace aym
