#ifndef AYM_CODEGEN_HELPERS_H
#define AYM_CODEGEN_HELPERS_H

#include <string>
#include <vector>

namespace aym {

std::string genLabel(const std::string &base);
std::string lowerName(const std::string &value);
std::string toAsmBytes(const std::string &value);
std::vector<std::string> paramRegs(bool windows);
std::string reg1(bool windows);
std::string reg2(bool windows);

} // namespace aym

#endif // AYM_CODEGEN_HELPERS_H
