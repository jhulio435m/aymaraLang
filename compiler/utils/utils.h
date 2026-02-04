#ifndef AYM_UTILS_H
#define AYM_UTILS_H

#include <string>

namespace aym {

std::string readFile(const std::string &path);
std::string executableDir();
std::string getEnvVar(const std::string &name);

} // namespace aym

#endif // AYM_UTILS_H
