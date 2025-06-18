#include "builtins.h"

namespace aym {

static const std::unordered_map<std::string, BuiltinInfo> builtins = {
    {BUILTIN_PRINT, {1, {}}},
    {BUILTIN_INPUT, {0, {}}},
    {BUILTIN_LENGTH, {1, {Type::String}}}
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
