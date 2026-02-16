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
    {BUILTIN_FABS, {1, {Type::Float}}},
    {BUILTIN_TO_STRING, {1, {}}},
    {BUILTIN_TO_NUMBER, {1, {}}},
    {BUILTIN_KATU, {2, {Type::String, Type::String}}},
    {BUILTIN_LARGO, {1, {Type::Int}}},
    {BUILTIN_PUSH, {2, {Type::Int, Type::Int}}},
    {BUILTIN_SUYU, {1, {Type::String}}},
    {BUILTIN_CHUSA, {1, {Type::String}}},
    {BUILTIN_JALJTA, {2, {Type::String, Type::String}}},
    {BUILTIN_MAYACHTA, {2, {Type::Int, Type::String}}},
    {BUILTIN_SIKTA, {3, {Type::String, Type::String, Type::String}}},
    {BUILTIN_UTJI, {2, {Type::String, Type::String}}},
    {BUILTIN_SUYUT, {1, {Type::Int}}},
    {BUILTIN_CHULLU, {2, {Type::Int, Type::Int}}},
    {BUILTIN_APSU, {1, {Type::Int}}},
    {BUILTIN_APSU_UKA, {2, {Type::Int, Type::Int}}},
    {BUILTIN_UTJIT, {2, {Type::Int, Type::Int}}},
    {BUILTIN_UTJI_SUTI, {2, {}}},
    {BUILTIN_SUYU_M, {1, {}}},
    {BUILTIN_SUTINAKA, {1, {}}},
    {BUILTIN_CHANINAKA, {1, {}}},
    {BUILTIN_APSU_SUTI, {2, {}}},
    {BUILTIN_CHANI_M, {2, {}}},
    {BUILTIN_PANTALLA_LIMPIA, {0, {}}},
    {BUILTIN_CURSOR_MOVER, {2, {Type::Int, Type::Int}}},
    {BUILTIN_COLOR, {2, {Type::Int, Type::Int}}},
    {BUILTIN_COLOR_RESTABLECER, {0, {}}},
    {BUILTIN_CURSOR_VISIBLE, {1, {Type::Int}}},
    {BUILTIN_TECLA, {0, {}}},
    {BUILTIN_TIEMPO_MS, {0, {}}},
    {BUILTIN_UJA_QALLTA, {3, {Type::Int, Type::Int, Type::String}}},
    {BUILTIN_UJA_UTJI, {0, {}}},
    {BUILTIN_UJA_PICHHA, {3, {Type::Int, Type::Int, Type::Int}}},
    {BUILTIN_UJA_SUYU, {7, {Type::Int, Type::Int, Type::Int, Type::Int, Type::Int, Type::Int, Type::Int}}},
    {BUILTIN_UJA_QILLQA, {6, {Type::String, Type::Int, Type::Int, Type::Int, Type::Int, Type::Int}}},
    {BUILTIN_UJA_USTAYA, {0, {}}},
    {BUILTIN_UJA_TUKUYA, {0, {}}},
    {BUILTIN_UJA_TECLA, {1, {Type::Int}}},
    {BUILTIN_ARG_CANTIDAD, {0, {}}},
    {BUILTIN_ARG_OBTENER, {1, {Type::Int}}},
    {BUILTIN_AFIRMA, {2, {Type::Int, Type::String}}},
    {BUILTIN_MAP, {2, {Type::Int, Type::Int}}},
    {BUILTIN_FILTER, {2, {Type::Int, Type::Int}}},
    {BUILTIN_REDUCE, {3, {Type::Int, Type::Int, Type::Int}}},
    {BUILTIN_MAYJTAYA, {2, {Type::Int, Type::Int}}},
    {BUILTIN_AJLLI, {2, {Type::Int, Type::Int}}},
    {BUILTIN_THAQTHAPI, {3, {Type::Int, Type::Int, Type::Int}}},
    {BUILTIN_ULLANA_ARU, {1, {Type::String}}},
    {BUILTIN_QILLQANA_ARU, {2, {Type::String, Type::String}}},
    {BUILTIN_UTJI_ARKATA, {1, {Type::String}}},
    {BUILTIN_WAKICHA, {1, {Type::Int}}},
    {BUILTIN_THAQHA, {2, {Type::Int, Type::Int}}},
    {BUILTIN_SAPAKI, {1, {Type::Int}}}
};

const std::unordered_map<std::string, BuiltinInfo> &getBuiltinFunctions() {
    return builtins;
}

std::string typeName(Type t) {
    switch (t) {
    case Type::Int:
        return "jakhüwi";
    case Type::Float:
        return "jakhüwi";
    case Type::Bool:
        return "chiqa";
    case Type::String:
        return "aru";
    }
    return "";
}

} // namespace aym
