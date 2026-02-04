#include "codegen_helpers.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace aym {

namespace {
size_t labelCounter = 0;
}

std::string genLabel(const std::string &base) {
    return base + std::to_string(labelCounter++);
}

std::string lowerName(const std::string &value) {
    std::string out = value;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return out;
}

std::string toAsmBytes(const std::string &value) {
    std::ostringstream oss;
    for (size_t i = 0; i < value.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << static_cast<unsigned int>(static_cast<unsigned char>(value[i]));
    }
    if (!value.empty()) oss << ", ";
    oss << "0";
    return oss.str();
}

std::vector<std::string> paramRegs(bool windows) {
    if (windows) {
        return {"rcx", "rdx", "r8", "r9"};
    }
    return {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
}

std::string reg1(bool windows) {
    return windows ? "rcx" : "rdi";
}

std::string reg2(bool windows) {
    return windows ? "rdx" : "rsi";
}

} // namespace aym
