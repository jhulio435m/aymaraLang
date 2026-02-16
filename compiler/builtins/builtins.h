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

constexpr const char BUILTIN_PRINT[] = "qillqa";
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
constexpr const char BUILTIN_TO_STRING[] = "aru";
constexpr const char BUILTIN_TO_NUMBER[] = "jakhüwi";
constexpr const char BUILTIN_KATU[] = "katu";
constexpr const char BUILTIN_LARGO[] = "largo";
constexpr const char BUILTIN_PUSH[] = "push";
constexpr const char BUILTIN_SUYU[] = "suyu";
constexpr const char BUILTIN_CHUSA[] = "ch'usa";
constexpr const char BUILTIN_JALJTA[] = "jaljta";
constexpr const char BUILTIN_MAYACHTA[] = "mayachta";
constexpr const char BUILTIN_SIKTA[] = "sikta";
constexpr const char BUILTIN_UTJI[] = "utji";
constexpr const char BUILTIN_SUYUT[] = "suyut";
constexpr const char BUILTIN_CHULLU[] = "ch'ullu";
constexpr const char BUILTIN_APSU[] = "apsu";
constexpr const char BUILTIN_APSU_UKA[] = "apsuuka";
constexpr const char BUILTIN_UTJIT[] = "utjit";
constexpr const char BUILTIN_UTJI_SUTI[] = "utjisuti";
constexpr const char BUILTIN_SUYU_M[] = "suyum";
constexpr const char BUILTIN_SUTINAKA[] = "sutinaka";
constexpr const char BUILTIN_CHANINAKA[] = "chaninaka";
constexpr const char BUILTIN_APSU_SUTI[] = "apsusuti";
constexpr const char BUILTIN_CHANI_M[] = "chanim";
constexpr const char BUILTIN_PANTALLA_LIMPIA[] = "pantalla_limpia";
constexpr const char BUILTIN_CURSOR_MOVER[] = "cursor_mover";
constexpr const char BUILTIN_COLOR[] = "color";
constexpr const char BUILTIN_COLOR_RESTABLECER[] = "color_restablecer";
constexpr const char BUILTIN_CURSOR_VISIBLE[] = "cursor_visible";
constexpr const char BUILTIN_TECLA[] = "tecla";
constexpr const char BUILTIN_TIEMPO_MS[] = "tiempo_ms";
constexpr const char BUILTIN_UJA_QALLTA[] = "uja_qallta";
constexpr const char BUILTIN_UJA_UTJI[] = "uja_utji";
constexpr const char BUILTIN_UJA_PICHHA[] = "uja_pichha";
constexpr const char BUILTIN_UJA_SUYU[] = "uja_suyu";
constexpr const char BUILTIN_UJA_QILLQA[] = "uja_qillqa";
constexpr const char BUILTIN_UJA_USTAYA[] = "uja_ustaya";
constexpr const char BUILTIN_UJA_TUKUYA[] = "uja_tukuya";
constexpr const char BUILTIN_UJA_TECLA[] = "uja_tecla";
constexpr const char BUILTIN_ARG_CANTIDAD[] = "arg_cantidad";
constexpr const char BUILTIN_ARG_OBTENER[] = "arg_obtener";
constexpr const char BUILTIN_AFIRMA[] = "afirma";
constexpr const char BUILTIN_MAP[] = "map";
constexpr const char BUILTIN_FILTER[] = "filter";
constexpr const char BUILTIN_REDUCE[] = "reduce";
constexpr const char BUILTIN_MAYJTAYA[] = "mayjtaya";
constexpr const char BUILTIN_AJLLI[] = "ajlli";
constexpr const char BUILTIN_THAQTHAPI[] = "thaqthapi";
constexpr const char BUILTIN_ULLANA_ARU[] = "ullana_aru";
constexpr const char BUILTIN_QILLQANA_ARU[] = "qillqana_aru";
constexpr const char BUILTIN_UTJI_ARKATA[] = "utji_arkata";
constexpr const char BUILTIN_WAKICHA[] = "wakicha";
constexpr const char BUILTIN_THAQHA[] = "thaqha";
constexpr const char BUILTIN_SAPAKI[] = "sapaki";

const std::unordered_map<std::string, BuiltinInfo> &getBuiltinFunctions();
std::string typeName(Type t);

} // namespace aym

#endif // AYM_BUILTINS_H
