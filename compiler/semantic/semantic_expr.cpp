#include "semantic.h"
#include "../utils/class_names.h"
#include "../builtins/builtins.h"
#include <algorithm>

namespace aym {

void SemanticAnalyzer::visit(NumberExpr &n) {
    markNode(n);
    currentType = "jakhüwi";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(BoolExpr &b) {
    markNode(b);
    currentType = "chiqa";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(StringExpr &s) {
    markNode(s);
    currentType = "aru";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(VariableExpr &v) {
    markNode(v);
    if (!isDeclared(v.getName())) {
        if (isClassName(v.getName())) {
            currentType = "kasta-ref:" + v.getName();
        } else {
            reportError("variable '" + v.getName() + "' no declarada", "AYM3002");
            currentType = "";
        }
    } else {
        currentType = lookup(v.getName());
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(BinaryExpr &b) {
    markNode(b);
    b.getLeft()->accept(*this);
    std::string l = currentType;
    b.getRight()->accept(*this);
    std::string r = currentType;
    if (l != r) {
        reportError("tipos incompatibles en operacion", "AYM3003");
    }
    char op = b.getOp();
    if ((l == "aru" || r == "aru") && op != '+' && op != 's' && op != 'd') {
        reportError("operacion invalida sobre textos", "AYM3003");
    }
    if (op=='&' || op=='|' || op=='s' || op=='d' || op=='<' || op=='>' || op=='l' || op=='g')
        currentType = "chiqa";
    else
        currentType = l;
    lastInputCall = false;
}

void SemanticAnalyzer::visit(UnaryExpr &u) {
    markNode(u);
    u.getExpr()->accept(*this);
    if (u.getOp() == '!') {
        currentType = "chiqa";
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(TernaryExpr &t) {
    markNode(t);
    t.getCondition()->accept(*this);
    std::string condType = currentType;
    if (condType != "chiqa") {
        reportError("condicion del ternario debe ser booleana", "AYM3003");
    }
    t.getThen()->accept(*this);
    std::string thenType = currentType;
    t.getElse()->accept(*this);
    std::string elseType = currentType;
    if (thenType != elseType) {
        reportError("tipos incompatibles en operador ternario", "AYM3003");
    }
    currentType = thenType;
    lastInputCall = false;
}

void SemanticAnalyzer::visit(IncDecExpr &e) {
    markNode(e);
    if (!isDeclared(e.getName())) {
        reportError("variable '" + e.getName() + "' no declarada", "AYM3002");
        currentType = "";
    } else {
        std::string t = lookup(e.getName());
        if (t != "jakhüwi") {
            reportError("incremento/decremento requiere numero", "AYM3003");
        }
        currentType = t;
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(FunctionRefExpr &f) {
    markNode(f);
    currentType = "jakhüwi";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(ListExpr &l) {
    markNode(l);
    std::string elementType;
    for (const auto &elem : l.getElements()) {
        elem->accept(*this);
        std::string t = currentType;
        if (elementType.empty()) {
            elementType = t;
        } else if (elementType != t) {
            reportError("tipos incompatibles en lista");
        }
    }
    if (elementType.empty()) {
        elementType = "jakhüwi";
    }
    currentType = "t'aqa:" + elementType;
    lastInputCall = false;
}

void SemanticAnalyzer::visit(MapExpr &m) {
    markNode(m);
    std::string valueType;
    bool sawValue = false;
    for (const auto &item : m.getItems()) {
        item.first->accept(*this);
        std::string keyType = currentType;
        if (keyType != "aru") {
            reportError("clave de mapa debe ser texto");
        }
        item.second->accept(*this);
        std::string t = currentType;
        if (!sawValue) {
            valueType = t;
            sawValue = true;
        } else if (!valueType.empty() && t != valueType) {
            valueType.clear();
        }
    }
    if (!sawValue) {
        currentType = "mapa:jakhüwi";
    } else if (!valueType.empty()) {
        currentType = "mapa:" + valueType;
    } else {
        currentType = "mapa";
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(IndexExpr &i) {
    markNode(i);
    i.getBase()->accept(*this);
    std::string baseType = currentType;
    i.getIndex()->accept(*this);
    if (baseType.rfind("t'aqa:", 0) == 0) {
        currentType = baseType.substr(6);
    } else if (baseType.rfind("mapa", 0) == 0) {
        if (baseType.rfind("mapa:", 0) == 0) {
            currentType = baseType.substr(5);
        } else {
            currentType = "jakhüwi";
        }
    } else {
        reportError("se esperaba una lista para indexacion");
        currentType = "";
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(MemberExpr &m) {
    markNode(m);
    m.getBase()->accept(*this);
    std::string baseType = currentType;
    if (baseType == "excepcion") {
        m.setExceptionAccess(true);
        currentType = "aru";
        m.setResolvedType("aru");
    } else if (baseType.rfind("kasta:", 0) == 0) {
        std::string className = baseType.substr(6);
        const FieldInfo *field = lookupField(className, m.getMember());
        if (!field) {
            reportError("atributo '" + m.getMember() + "' no existe en '" + className + "'");
            currentType = "";
        } else if (field->isPrivate && currentClass != field->ownerClass) {
            reportError("atributo privado '" + m.getMember() + "' no accesible");
            currentType = "";
        } else {
            currentType = field->type;
            m.setResolvedType(field->type);
        }
    } else if (baseType.rfind("kasta-ref:", 0) == 0) {
        std::string className = baseType.substr(10);
        const FieldInfo *field = lookupStaticField(className, m.getMember());
        if (!field) {
            reportError("atributo estatico '" + m.getMember() + "' no existe en '" + className + "'");
            currentType = "";
        } else if (field->isPrivate && currentClass != field->ownerClass) {
            reportError("atributo estatico privado '" + m.getMember() + "' no accesible");
            currentType = "";
        } else {
            m.setStaticField(classStaticFieldName(className, m.getMember()));
            currentType = field->type;
            m.setResolvedType(field->type);
        }
    } else if (baseType.rfind("mapa:", 0) == 0) {
        std::string valueType = baseType.substr(5);
        if (valueType.empty()) valueType = "jakhüwi";
        currentType = valueType;
        m.setResolvedType(valueType);
    } else if (baseType == "mapa") {
        currentType = "jakhüwi";
        m.setResolvedType("jakhüwi");
    } else {
        reportError("acceso de miembro invalido");
        currentType = "";
    }
    lastInputCall = false;
}

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
            const std::string &expectedType = pit->second[idx];
            if (!expectedType.empty() && !t.empty() && !isTypeAssignable(t, expectedType)) {
                reportError("tipo incompatible en argumento " + std::to_string(idx + 1) +
                                " en llamada a '" + c.getName() + "'",
                            "AYM3003");
            }
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
