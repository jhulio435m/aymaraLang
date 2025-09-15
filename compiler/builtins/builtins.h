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
constexpr const char BUILTIN_WRITE[] = "write";
constexpr const char BUILTIN_SLEEP[] = "sleep";
constexpr const char BUILTIN_ARRAY_NEW[] = "array";
constexpr const char BUILTIN_ARRAY_GET[] = "array_get";
constexpr const char BUILTIN_ARRAY_SET[] = "array_set";
constexpr const char BUILTIN_ARRAY_FREE[] = "array_free";
constexpr const char BUILTIN_ARRAY_LENGTH[] = "array_length";
constexpr const char BUILTIN_SIN[] = "sin";
constexpr const char BUILTIN_COS[] = "cos";
constexpr const char BUILTIN_TAN[] = "tan";
constexpr const char BUILTIN_ASIN[] = "asin";
constexpr const char BUILTIN_ACOS[] = "acos";
constexpr const char BUILTIN_ATAN[] = "atan";
constexpr const char BUILTIN_SQRT[] = "sqrt";
constexpr const char BUILTIN_POW[] = "pow";
constexpr const char BUILTIN_EXP[] = "exp";
constexpr const char BUILTIN_LOG[] = "log";
constexpr const char BUILTIN_LOG10[] = "log10";
constexpr const char BUILTIN_FLOOR[] = "floor";
constexpr const char BUILTIN_CEIL[] = "ceil";
constexpr const char BUILTIN_ROUND[] = "round";
constexpr const char BUILTIN_FABS[] = "fabs";

const std::unordered_map<std::string, BuiltinInfo> &getBuiltinFunctions();
std::string typeName(Type t);

} // namespace aym

#endif // AYM_BUILTINS_H
