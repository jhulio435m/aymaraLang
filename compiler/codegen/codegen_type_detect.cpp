#include "codegen_impl.h"
#include "../builtins/builtins.h"

namespace aym {

bool CodeGenImpl::isStringExpr(const Expr *expr,
                               const std::unordered_map<std::string,int> *locals) const {
    if (!expr) return false;
    if (dynamic_cast<const StringExpr*>(expr)) return true;
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        std::string nameLower = lowerName(c->getName());
        if (nameLower == BUILTIN_TO_STRING || nameLower == BUILTIN_KATU ||
            nameLower == BUILTIN_CHUSA || nameLower == BUILTIN_MAYACHTA ||
            nameLower == BUILTIN_SIKTA || nameLower == BUILTIN_ARG_OBTENER ||
            nameLower == BUILTIN_ULLANA_ARU) {
            return true;
        }
        if (nameLower == BUILTIN_CHANI_M) {
            if (c->getArgs().size() == 3) {
                return isStringExpr(c->getArgs()[2].get(), locals);
            }
            if (!c->getArgs().empty()) {
                return mapValueType(c->getArgs()[0].get(), locals) == "aru";
            }
        }
        if ((nameLower == BUILTIN_APSU || nameLower == BUILTIN_APSU_UKA) &&
            !c->getArgs().empty() &&
            listElementType(c->getArgs()[0].get(), locals) == "aru") {
            return true;
        }
        auto it = functionReturnTypes.find(c->getName());
        if (it == functionReturnTypes.end()) it = functionReturnTypes.find(nameLower);
        if (it != functionReturnTypes.end() && it->second == "aru") return true;
    }
    if (auto *i = dynamic_cast<const IndexExpr*>(expr)) {
        if (isMapExpr(i->getBase(), locals)) {
            return mapValueType(i->getBase(), locals) == "aru";
        }
        return listElementType(i->getBase(), locals) == "aru";
    }
    if (auto *m = dynamic_cast<const MemberExpr*>(expr)) {
        if (!m->getResolvedType().empty()) {
            return m->getResolvedType() == "aru";
        }
        return true;
    }
    if (auto *m = dynamic_cast<const MemberCallExpr*>(expr)) {
        if (!m->getResolvedType().empty()) {
            return m->getResolvedType() == "aru";
        }
    }
    if (auto *v = dynamic_cast<const VariableExpr*>(expr)) {
        if (currentParamStrings.count(v->getName()) && currentParamStrings.at(v->getName())) return true;
        if (currentLocalStrings.count(v->getName()) && currentLocalStrings.at(v->getName())) return true;
        if (globalTypes.count(v->getName()) && globalTypes.at(v->getName()) == "aru") return true;
        return false;
    }
    if (auto *b = dynamic_cast<const BinaryExpr*>(expr)) {
        if (b->getOp() == '+') {
            return isStringExpr(b->getLeft(), locals) && isStringExpr(b->getRight(), locals);
        }
        return false;
    }
    if (auto *t = dynamic_cast<const TernaryExpr*>(expr)) {
        return isStringExpr(t->getThen(), locals) && isStringExpr(t->getElse(), locals);
    }
    return false;
}

bool CodeGenImpl::isListExpr(const Expr *expr,
                             const std::unordered_map<std::string,int> *locals) const {
    (void)locals;
    if (!expr) return false;
    if (dynamic_cast<const ListExpr*>(expr)) return true;
    if (auto *v = dynamic_cast<const VariableExpr*>(expr)) {
        auto it = globalTypes.find(v->getName());
        if (it != globalTypes.end() && it->second.rfind("t'aqa", 0) == 0) return true;
        if (currentParamTypes.count(v->getName()) && currentParamTypes.at(v->getName()).rfind("t'aqa", 0) == 0)
            return true;
        if (currentLocalTypes.count(v->getName()) &&
            currentLocalTypes.at(v->getName()).rfind("t'aqa", 0) == 0)
            return true;
    }
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        std::string name = lowerName(c->getName());
        if (name == BUILTIN_PUSH || name == BUILTIN_CHULLU || name == BUILTIN_JALJTA ||
            name == BUILTIN_SUTINAKA || name == BUILTIN_CHANINAKA ||
            name == BUILTIN_WAKICHA || name == BUILTIN_SAPAKI) return true;
        auto it = functionReturnTypes.find(c->getName());
        if (it == functionReturnTypes.end()) it = functionReturnTypes.find(name);
        if (it != functionReturnTypes.end() && it->second.rfind("t'aqa", 0) == 0) return true;
    }
    return false;
}

bool CodeGenImpl::isMapExpr(const Expr *expr,
                            const std::unordered_map<std::string,int> *locals) const {
    (void)locals;
    if (!expr) return false;
    if (dynamic_cast<const MapExpr*>(expr)) return true;
    if (auto *v = dynamic_cast<const VariableExpr*>(expr)) {
        auto it = globalTypes.find(v->getName());
        if (it != globalTypes.end() && it->second.rfind("mapa", 0) == 0) return true;
        if (it != globalTypes.end() && it->second.rfind("kasta:", 0) == 0) return true;
        if (currentParamTypes.count(v->getName()) &&
            currentParamTypes.at(v->getName()).rfind("mapa", 0) == 0)
            return true;
        if (currentParamTypes.count(v->getName()) &&
            currentParamTypes.at(v->getName()).rfind("kasta:", 0) == 0)
            return true;
        if (currentLocalTypes.count(v->getName()) &&
            currentLocalTypes.at(v->getName()).rfind("mapa", 0) == 0)
            return true;
        if (currentLocalTypes.count(v->getName()) &&
            currentLocalTypes.at(v->getName()).rfind("kasta:", 0) == 0)
            return true;
    }
    if (dynamic_cast<const NewExpr*>(expr)) {
        return true;
    }
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        std::string name = lowerName(c->getName());
        auto it = functionReturnTypes.find(c->getName());
        if (it == functionReturnTypes.end()) it = functionReturnTypes.find(name);
        if (it != functionReturnTypes.end() && it->second.rfind("mapa", 0) == 0) return true;
    }
    return false;
}

std::string CodeGenImpl::listElementType(const Expr *expr,
                                         const std::unordered_map<std::string,int> *locals) const {
    if (!expr) return "";
    if (auto *list = dynamic_cast<const ListExpr*>(expr)) {
        bool seenString = false;
        bool seenNumber = false;
        for (const auto &elem : list->getElements()) {
            if (isStringExpr(elem.get(), locals)) {
                seenString = true;
            } else {
                seenNumber = true;
            }
        }
        if (seenString && !seenNumber) return "aru";
        return "jakhüwi";
    }
    if (auto *v = dynamic_cast<const VariableExpr*>(expr)) {
        auto checkType = [&](const std::unordered_map<std::string,std::string> &types) -> std::string {
            auto it = types.find(v->getName());
            if (it != types.end()) {
                if (it->second.rfind("t'aqa:", 0) == 0) {
                    return it->second.substr(6);
                }
                if (it->second == "t'aqa") {
                    return "jakhüwi";
                }
            }
            return "";
        };
        std::string t;
        if (!currentParamTypes.empty()) {
            t = checkType(currentParamTypes);
            if (!t.empty()) return t;
        }
        if (!currentLocalTypes.empty()) {
            t = checkType(currentLocalTypes);
            if (!t.empty()) return t;
        }
        t = checkType(globalTypes);
        if (!t.empty()) return t;
    }
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        std::string name = lowerName(c->getName());
        if (name == BUILTIN_JALJTA) {
            return "aru";
        }
        if (name == BUILTIN_SUTINAKA) {
            return "aru";
        }
        if (name == BUILTIN_CHANINAKA && !c->getArgs().empty()) {
            return mapValueType(c->getArgs()[0].get(), locals);
        }
        if ((name == BUILTIN_PUSH || name == BUILTIN_CHULLU) && !c->getArgs().empty()) {
            return listElementType(c->getArgs()[0].get(), locals);
        }
        if ((name == BUILTIN_WAKICHA || name == BUILTIN_SAPAKI) && !c->getArgs().empty()) {
            return listElementType(c->getArgs()[0].get(), locals);
        }
    }
    return "";
}

std::string CodeGenImpl::mapValueType(const Expr *expr,
                                      const std::unordered_map<std::string,int> *locals) const {
    if (!expr) return "";
    if (auto *map = dynamic_cast<const MapExpr*>(expr)) {
        bool seenString = false;
        bool seenNumber = false;
        for (const auto &item : map->getItems()) {
            if (isStringExpr(item.second.get(), locals)) {
                seenString = true;
            } else {
                seenNumber = true;
            }
        }
        if (seenString && !seenNumber) return "aru";
        return "jakhüwi";
    }
    if (auto *v = dynamic_cast<const VariableExpr*>(expr)) {
        auto checkType = [&](const std::unordered_map<std::string,std::string> &types) -> std::string {
            auto it = types.find(v->getName());
            if (it != types.end()) {
                if (it->second.rfind("mapa:", 0) == 0) {
                    return it->second.substr(5);
                }
                if (it->second == "mapa") {
                    return "jakhüwi";
                }
            }
            return "";
        };
        std::string t;
        if (!currentParamTypes.empty()) {
            t = checkType(currentParamTypes);
            if (!t.empty()) return t;
        }
        if (!currentLocalTypes.empty()) {
            t = checkType(currentLocalTypes);
            if (!t.empty()) return t;
        }
        t = checkType(globalTypes);
        if (!t.empty()) return t;
    }
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        std::string name = lowerName(c->getName());
        auto it = functionReturnTypes.find(c->getName());
        if (it == functionReturnTypes.end()) it = functionReturnTypes.find(name);
        if (it != functionReturnTypes.end() && it->second.rfind("mapa:", 0) == 0) {
            return it->second.substr(5);
        }
    }
    return "";
}

bool CodeGenImpl::isBoolExpr(const Expr *expr,
                             const std::unordered_map<std::string,int> *locals) const {
    if (!expr) return false;
    if (dynamic_cast<const BoolExpr*>(expr)) return true;
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        std::string nameLower = lowerName(c->getName());
        if (nameLower == BUILTIN_UTJI || nameLower == BUILTIN_UTJIT ||
            nameLower == BUILTIN_UTJI_SUTI) return true;
        auto it = functionReturnTypes.find(c->getName());
        if (it == functionReturnTypes.end()) it = functionReturnTypes.find(nameLower);
        if (it != functionReturnTypes.end() && it->second == "chiqa") return true;
    }
    if (auto *v = dynamic_cast<const VariableExpr*>(expr)) {
        if (currentParamTypes.count(v->getName()) && currentParamTypes.at(v->getName()) == "chiqa")
            return true;
        if (locals && currentLocalTypes.count(v->getName()) &&
            currentLocalTypes.at(v->getName()) == "chiqa")
            return true;
        if (globalTypes.count(v->getName()) && globalTypes.at(v->getName()) == "chiqa")
            return true;
        return false;
    }
    if (auto *b = dynamic_cast<const BinaryExpr*>(expr)) {
        char op = b->getOp();
        if (op == '&' || op == '|' || op == 's' || op == 'd' || op == '<' || op == '>' ||
            op == 'l' || op == 'g') {
            return true;
        }
        return false;
    }
    if (auto *u = dynamic_cast<const UnaryExpr*>(expr)) {
        return u->getOp() == '!';
    }
    if (auto *t = dynamic_cast<const TernaryExpr*>(expr)) {
        return isBoolExpr(t->getThen(), locals) && isBoolExpr(t->getElse(), locals);
    }
    return false;
}

} // namespace aym
