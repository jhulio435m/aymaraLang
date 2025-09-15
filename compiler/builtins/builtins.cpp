#include "builtins.h"

namespace aym {

static const std::unordered_map<std::string, BuiltinInfo> builtins = {
    {BUILTIN_PRINT, {1, {}}},
    {BUILTIN_INPUT, {0, {}}},
    {BUILTIN_LENGTH, {1, {Type::String}}},
    {BUILTIN_RANDOM, {1, {Type::Int}}},
    {BUILTIN_SLEEP, {1, {Type::Int}}},
    {BUILTIN_ARRAY_NEW, {1, {Type::Int}}},
    {BUILTIN_ARRAY_GET, {2, {Type::Int, Type::Int}}},
    {BUILTIN_ARRAY_SET, {3, {Type::Int, Type::Int, Type::Int}}},
    {BUILTIN_ARRAY_FREE, {1, {Type::Int}}},
    {BUILTIN_ARRAY_LENGTH, {1, {Type::Int}}},
    {BUILTIN_WRITE, {1, {Type::String}}},
    {BUILTIN_SIN, {1, {Type::Float}}},
    {BUILTIN_COS, {1, {Type::Float}}},
    {BUILTIN_TAN, {1, {Type::Float}}},
    {BUILTIN_ASIN, {1, {Type::Float}}},
    {BUILTIN_ACOS, {1, {Type::Float}}},
    {BUILTIN_ATAN, {1, {Type::Float}}},
    {BUILTIN_SQRT, {1, {Type::Float}}},
    {BUILTIN_POW, {2, {Type::Float, Type::Float}}},
    {BUILTIN_EXP, {1, {Type::Float}}},
    {BUILTIN_LOG, {1, {Type::Float}}},
    {BUILTIN_LOG10, {1, {Type::Float}}},
    {BUILTIN_FLOOR, {1, {Type::Float}}},
    {BUILTIN_CEIL, {1, {Type::Float}}},
    {BUILTIN_ROUND, {1, {Type::Float}}},
    {BUILTIN_FABS, {1, {Type::Float}}}
};

const std::unordered_map<std::string, BuiltinInfo> &getBuiltinFunctions() {
    return builtins;
}

std::string typeName(Type t) {
    switch (t) {
    case Type::Int:
        return "jach’a";
    case Type::Float:
        return "lliphiphi";
    case Type::Bool:
        return "chuymani";
    case Type::String:
        return "qillqa";
    }
    return "";
}

} // namespace aym
