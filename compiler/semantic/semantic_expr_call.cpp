#include "semantic.h"
#include "../builtins/builtins.h"
#include <algorithm>

namespace aym {

void SemanticAnalyzer::visit(CallExpr &c) {
    markNode(c);
    std::string nameLower = c.getName();
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    auto it = functions.find(c.getName());
    if (it == functions.end()) {
        it = functions.find(nameLower);
    }
    if (it == functions.end()) {
        reportError("funcion '" + c.getName() + "' no declarada", "AYM3004");
    } else if (nameLower == "katu") {
        if (c.getArgs().size() < 1 || c.getArgs().size() > 2) {
            reportError("numero incorrecto de argumentos en llamada a '" + c.getName() + "'", "AYM3005");
        }
    } else if (nameLower == BUILTIN_CHANI_M) {
        if (c.getArgs().size() < 2 || c.getArgs().size() > 3) {
            reportError("numero incorrecto de argumentos en llamada a '" + c.getName() + "'", "AYM3005");
        }
    } else if (c.getArgs().size() != it->second) {
        reportError("numero incorrecto de argumentos en llamada a '" + c.getName() + "'", "AYM3005");
    }
    size_t idx = 0;
    auto pit = paramTypes.find(c.getName());
    if (pit == paramTypes.end()) {
        pit = paramTypes.find(nameLower);
    }
    for (const auto &arg : c.getArgs()) {
        arg->accept(*this);
        std::string t = currentType;
        if (pit != paramTypes.end() && idx < pit->second.size()) {
            if (t == "aru") pit->second[idx] = "aru";
        }
        ++idx;
    }
    if (nameLower == BUILTIN_TO_STRING || nameLower == BUILTIN_CHUSA ||
        nameLower == BUILTIN_MAYACHTA || nameLower == BUILTIN_SIKTA ||
        nameLower == BUILTIN_ULLANA_ARU) {
        currentType = "aru";
    } else if (nameLower == BUILTIN_TO_NUMBER) {
        currentType = "jakhüwi";
    } else if (nameLower == BUILTIN_ARG_OBTENER) {
        currentType = "aru";
    } else if (nameLower == "katu") {
        currentType = "aru";
    } else if (nameLower == "largo" || nameLower == BUILTIN_SUYU || nameLower == BUILTIN_SUYUT) {
        currentType = "jakhüwi";
    } else if (nameLower == BUILTIN_SUYU_M) {
        currentType = "jakhüwi";
    } else if (nameLower == BUILTIN_UJA_QALLTA || nameLower == BUILTIN_UJA_UTJI ||
               nameLower == BUILTIN_UJA_USTAYA || nameLower == BUILTIN_UJA_TUKUYA ||
               nameLower == BUILTIN_UJA_TECLA || nameLower == BUILTIN_UJA_PICHHA ||
               nameLower == BUILTIN_UJA_SUYU || nameLower == BUILTIN_UJA_QILLQA) {
        currentType = "chiqa";
    } else if (nameLower == BUILTIN_ARG_CANTIDAD || nameLower == BUILTIN_AFIRMA ||
               nameLower == BUILTIN_QILLQANA_ARU || nameLower == BUILTIN_UTJI_ARKATA) {
        currentType = "jakhüwi";
    } else if (nameLower == BUILTIN_THAQHA) {
        currentType = "jakhüwi";
    } else if (nameLower == BUILTIN_UTJI || nameLower == BUILTIN_UTJIT || nameLower == BUILTIN_UTJI_SUTI) {
        currentType = "chiqa";
    } else if (nameLower == BUILTIN_MAP || nameLower == BUILTIN_FILTER ||
               nameLower == BUILTIN_MAYJTAYA || nameLower == BUILTIN_AJLLI ||
               nameLower == BUILTIN_WAKICHA || nameLower == BUILTIN_SAPAKI) {
        if (!c.getArgs().empty()) {
            c.getArgs()[0]->accept(*this);
            std::string baseType = currentType;
            if (baseType.rfind("t'aqa:", 0) == 0) {
                currentType = baseType;
            } else {
                currentType = "t'aqa:jakhüwi";
            }
        } else {
            currentType = "t'aqa:jakhüwi";
        }
    } else if (nameLower == BUILTIN_REDUCE || nameLower == BUILTIN_THAQTHAPI) {
        if (c.getArgs().size() >= 3) {
            c.getArgs()[2]->accept(*this);
            // keep currentType from initial accumulator
        } else {
            currentType = "jakhüwi";
        }
    } else if (nameLower == BUILTIN_JALJTA) {
        currentType = "t'aqa:aru";
    } else if (nameLower == BUILTIN_SUTINAKA) {
        currentType = "t'aqa:aru";
    } else if (nameLower == BUILTIN_CHANINAKA) {
        if (!c.getArgs().empty()) {
            c.getArgs()[0]->accept(*this);
            std::string baseType = currentType;
            if (baseType.rfind("mapa:", 0) == 0) {
                currentType = "t'aqa:" + baseType.substr(5);
            } else {
                currentType = "t'aqa:jakhüwi";
            }
        } else {
            currentType = "t'aqa:jakhüwi";
        }
    } else if (nameLower == BUILTIN_CHANI_M) {
        if (c.getArgs().size() == 3) {
            c.getArgs()[2]->accept(*this);
            currentType = currentType;
        } else if (!c.getArgs().empty()) {
            c.getArgs()[0]->accept(*this);
            std::string baseType = currentType;
            if (baseType.rfind("mapa:", 0) == 0) {
                currentType = baseType.substr(5);
            } else {
                currentType = "jakhüwi";
            }
        } else {
            currentType = "jakhüwi";
        }
    } else if (nameLower == "push" || nameLower == BUILTIN_CHULLU) {
        if (!c.getArgs().empty()) {
            c.getArgs()[0]->accept(*this);
            std::string baseType = currentType;
            if (c.getArgs().size() > 1) {
                c.getArgs()[1]->accept(*this);
                std::string valueType = currentType;
                if (baseType.rfind("t'aqa:", 0) == 0) {
                    std::string elementType = baseType.substr(6);
                    if (!elementType.empty() && valueType != elementType) {
                        reportError("tipo incompatible en push");
                    }
                    currentType = baseType;
                } else {
                    reportError("se esperaba una lista para push");
                }
            } else {
                currentType = baseType;
            }
        } else {
            currentType = "t'aqa:jakhüwi";
        }
    } else if (nameLower == BUILTIN_APSU || nameLower == BUILTIN_APSU_UKA) {
        if (!c.getArgs().empty()) {
            c.getArgs()[0]->accept(*this);
            std::string baseType = currentType;
            if (baseType.rfind("t'aqa:", 0) == 0) {
                currentType = baseType.substr(6);
            } else {
                reportError("se esperaba una lista para " + c.getName());
                currentType = "";
            }
        } else {
            currentType = "";
        }
    } else {
        auto fit = functionReturnTypes.find(c.getName());
        if (fit == functionReturnTypes.end()) {
            fit = functionReturnTypes.find(nameLower);
        }
        if (fit != functionReturnTypes.end() && !fit->second.empty()) {
            currentType = fit->second;
        } else {
            currentType = "jakhüwi";
        }
    }
    lastInputCall = (nameLower == BUILTIN_INPUT);
}

} // namespace aym
