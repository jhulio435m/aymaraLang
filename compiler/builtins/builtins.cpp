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
    {BUILTIN_JAKHU_CHANI, {1, {Type::Int}}},
    {BUILTIN_JAKHU_JISKA, {2, {Type::Int, Type::Int}}},
    {BUILTIN_JAKHU_JACHA, {2, {Type::Int, Type::Int}}},
    {BUILTIN_JAKHU_MCD, {2, {Type::Int, Type::Int}}},
    {BUILTIN_JAKHU_LIMITA, {3, {Type::Int, Type::Int, Type::Int}}},
    {BUILTIN_TO_STRING, {1, {}}},
    {BUILTIN_TO_NUMBER, {1, {}}},
    {BUILTIN_KATU, {2, {Type::String, Type::String}}},
    {BUILTIN_LARGO, {1, {}}},
    {BUILTIN_PUSH, {2, {}}},
    {BUILTIN_SUYU, {1, {Type::String}}},
    {BUILTIN_CHUSA, {1, {Type::String}}},
    {BUILTIN_JALJTA, {2, {Type::String, Type::String}}},
    {BUILTIN_MAYACHTA, {2, {}}},
    {BUILTIN_SIKTA, {3, {Type::String, Type::String, Type::String}}},
    {BUILTIN_UTJI, {2, {Type::String, Type::String}}},
    {BUILTIN_JACHANCHAYA, {1, {Type::String}}},
    {BUILTIN_JISKANCHAYA, {1, {Type::String}}},
    {BUILTIN_KUTIKIPA, {1, {Type::String}}},
    {BUILTIN_KUTI_ARU, {2, {Type::String, Type::Int}}},
    {BUILTIN_QALLTATI, {2, {Type::String, Type::String}}},
    {BUILTIN_TUKUYATI, {2, {Type::String, Type::String}}},
    {BUILTIN_JIKXATA, {2, {Type::String, Type::String}}},
    {BUILTIN_ARU_CHIMPU, {2, {Type::String, Type::Int}}},
    {BUILTIN_JALJA_ARU, {3, {Type::String, Type::Int, Type::Int}}},
    {BUILTIN_CHIMPU_JAKHUWI, {1, {Type::String}}},
    {BUILTIN_JAKHUWI_CHIMPU, {1, {Type::Int}}},
    {BUILTIN_SUYUT, {1, {}}},
    {BUILTIN_CHULLU, {2, {}}},
    {BUILTIN_APSU, {1, {}}},
    {BUILTIN_APSU_UKA, {2, {}}},
    {BUILTIN_UTJIT, {2, {}}},
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
    {BUILTIN_CURSOR_VISIBLE, {1, {Type::Bool}}},
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
    {BUILTIN_AFIRMA, {2, {Type::Bool, Type::String}}},
    {BUILTIN_PACHA_ARU, {1, {Type::String}}},
    {BUILTIN_MISTUNA, {1, {Type::Int}}},
    {BUILTIN_MAP, {2, {}}},
    {BUILTIN_FILTER, {2, {}}},
    {BUILTIN_REDUCE, {3, {}}},
    {BUILTIN_MAYJTAYA, {2, {}}},
    {BUILTIN_AJLLI, {2, {}}},
    {BUILTIN_THAQTHAPI, {3, {}}},
    {BUILTIN_ULLANA_ARU, {1, {Type::String}}},
    {BUILTIN_QILLQANA_ARU, {2, {Type::String, Type::String}}},
    {BUILTIN_UTJI_ARKATA, {1, {Type::String}}},
    {BUILTIN_YAPXATA_ARU, {2, {Type::String, Type::String}}},
    {BUILTIN_ULLANA_SIQI, {1, {Type::String}}},
    {BUILTIN_PICHHA_ARKATA, {1, {Type::String}}},
    {BUILTIN_WAKICHA, {1, {}}},
    {BUILTIN_THAQHA, {2, {}}},
    {BUILTIN_SAPAKI, {1, {}}},
    {BUILTIN_KUTIKIPAT, {1, {}}},
    {BUILTIN_JALJAT, {3, {}}},
    {BUILTIN_THANTA, {1, {}}},
    {BUILTIN_JISKA_TAQA, {1, {}}},
    {BUILTIN_JACHA_TAQA, {1, {}}},
    {BUILTIN_WAKICHA_QHIPA, {1, {}}}
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
