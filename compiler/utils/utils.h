#ifndef AYM_UTILS_H
#define AYM_UTILS_H

#include <string>
#include "fs.h"

namespace aym {

std::string readFile(const std::string &path);
fs::path executablePath();
fs::path executableDirPath();
std::string executableDir();
std::string getEnvVar(const std::string &name);
fs::path findBundledToolExecutable(const std::string &toolName);
std::string resolveToolExecutable(const std::string &toolName);

} // namespace aym

#endif // AYM_UTILS_H
