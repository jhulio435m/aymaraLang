#ifndef AYM_BUILTINS_H
#define AYM_BUILTINS_H

#include <string>
#include <vector>
#include <unordered_map>

namespace aym {

enum class Type {
    Int,
    Float,
    Bool,
    String
};

struct BuiltinInfo {
    size_t argCount;
    std::vector<Type> paramTypes;
};

constexpr const char BUILTIN_PRINT[] = "willt’aña";
constexpr const char BUILTIN_INPUT[] = "input";
constexpr const char BUILTIN_LENGTH[] = "length";
constexpr const char BUILTIN_RANDOM[] = "random";

const std::unordered_map<std::string, BuiltinInfo> &getBuiltinFunctions();
std::string typeName(Type t);

} // namespace aym

#endif // AYM_BUILTINS_H
