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
            nameLower == BUILTIN_SIKTA || nameLower == BUILTIN_JACHANCHAYA ||
            nameLower == BUILTIN_JISKANCHAYA || nameLower == BUILTIN_KUTIKIPA ||
            nameLower == BUILTIN_KUTI_ARU || nameLower == BUILTIN_ARG_OBTENER ||
            nameLower == BUILTIN_ULLANA_ARU || nameLower == BUILTIN_ARU_CHIMPU ||
            nameLower == BUILTIN_JALJA_ARU || nameLower == BUILTIN_JAKHUWI_CHIMPU ||
            nameLower == BUILTIN_PACHA_ARU) {
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
        if (isStringExpr(i->getBase(), locals)) {
            return true;
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
    if (auto *m = dynamic_cast<const MemberExpr*>(expr)) {
        return m->getResolvedType().rfind("t'aqa", 0) == 0;
    }
    if (auto *m = dynamic_cast<const MemberCallExpr*>(expr)) {
        return m->getResolvedType().rfind("t'aqa", 0) == 0;
    }
    if (auto *i = dynamic_cast<const IndexExpr*>(expr)) {
        if (isMapExpr(i->getBase(), locals)) {
            return mapValueType(i->getBase(), locals).rfind("t'aqa", 0) == 0;
        }
        return listElementType(i->getBase(), locals).rfind("t'aqa", 0) == 0;
    }
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        std::string name = lowerName(c->getName());
        if (name == BUILTIN_PUSH || name == BUILTIN_CHULLU || name == BUILTIN_JALJTA ||
            name == BUILTIN_SUTINAKA || name == BUILTIN_CHANINAKA ||
            name == BUILTIN_WAKICHA || name == BUILTIN_WAKICHA_QHIPA ||
            name == BUILTIN_SAPAKI || name == BUILTIN_KUTIKIPAT ||
            name == BUILTIN_JALJAT || name == BUILTIN_ULLANA_SIQI ||
            name == BUILTIN_MAP || name == BUILTIN_FILTER ||
            name == BUILTIN_MAYJTAYA || name == BUILTIN_AJLLI) return true;
        auto it = functionReturnTypes.find(c->getName());
        if (it == functionReturnTypes.end()) it = functionReturnTypes.find(name);
        if (it != functionReturnTypes.end() && it->second.rfind("t'aqa", 0) == 0) return true;
    }
    if (auto *t = dynamic_cast<const TernaryExpr*>(expr)) {
        return isListExpr(t->getThen(), locals) && isListExpr(t->getElse(), locals);
    }
    return false;
}

bool CodeGenImpl::isMapExpr(const Expr *expr,
                            const std::unordered_map<std::string,int> *locals) const {
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
    if (auto *m = dynamic_cast<const MemberExpr*>(expr)) {
        return m->getResolvedType().rfind("mapa", 0) == 0 ||
               m->getResolvedType().rfind("kasta:", 0) == 0;
    }
    if (auto *m = dynamic_cast<const MemberCallExpr*>(expr)) {
        return m->getResolvedType().rfind("mapa", 0) == 0 ||
               m->getResolvedType().rfind("kasta:", 0) == 0;
    }
    if (auto *i = dynamic_cast<const IndexExpr*>(expr)) {
        std::string inner = isMapExpr(i->getBase(), locals)
            ? mapValueType(i->getBase(), locals)
            : listElementType(i->getBase(), locals);
        return inner.rfind("mapa", 0) == 0 || inner.rfind("kasta:", 0) == 0;
    }
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        std::string name = lowerName(c->getName());
        auto it = functionReturnTypes.find(c->getName());
        if (it == functionReturnTypes.end()) it = functionReturnTypes.find(name);
        if (it != functionReturnTypes.end() &&
            (it->second.rfind("mapa", 0) == 0 || it->second.rfind("kasta:", 0) == 0)) return true;
    }
    if (auto *t = dynamic_cast<const TernaryExpr*>(expr)) {
        return isMapExpr(t->getThen(), locals) && isMapExpr(t->getElse(), locals);
    }
    return false;
}

std::string CodeGenImpl::listElementType(const Expr *expr,
                                         const std::unordered_map<std::string,int> *locals) const {
    if (!expr) return "";
    if (auto *list = dynamic_cast<const ListExpr*>(expr)) {
        if (!list->getElements().empty() && isListExpr(list->getElements()[0].get(), locals)) {
            std::string elem;
            for (const auto &el : list->getElements()) {
                std::string candidate = listElementType(el.get(), locals);
                if (auto *sub = dynamic_cast<const ListExpr*>(el.get())) {
                    if (!sub->getElements().empty()) {
                        elem = candidate;
                        break;
                    }
                } else if (!candidate.empty()) {
                    elem = candidate;
                    break;
                }
                if (elem.empty()) elem = candidate;
            }
            return elem.empty() ? "t'aqa:jakhüwi" : "t'aqa:" + elem;
        }
        if (!list->getElements().empty() && isMapExpr(list->getElements()[0].get(), locals)) {
            std::string val = mapValueType(list->getElements()[0].get(), locals);
            return val.empty() ? "mapa" : "mapa:" + val;
        }
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
    if (auto *m = dynamic_cast<const MemberExpr*>(expr)) {
        if (m->getResolvedType().rfind("t'aqa:", 0) == 0) {
            return m->getResolvedType().substr(6);
        }
        if (m->getResolvedType() == "t'aqa") {
            return "jakhüwi";
        }
    }
    if (auto *m = dynamic_cast<const MemberCallExpr*>(expr)) {
        if (m->getResolvedType().rfind("t'aqa:", 0) == 0) {
            return m->getResolvedType().substr(6);
        }
        if (m->getResolvedType() == "t'aqa") {
            return "jakhüwi";
        }
    }
    if (auto *i = dynamic_cast<const IndexExpr*>(expr)) {
        std::string inner = isMapExpr(i->getBase(), locals)
            ? mapValueType(i->getBase(), locals)
            : listElementType(i->getBase(), locals);
        if (inner.rfind("t'aqa:", 0) == 0) {
            return inner.substr(6);
        }
        if (inner == "t'aqa") {
            return "jakhüwi";
        }
    }
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        std::string name = lowerName(c->getName());
        if (name == BUILTIN_JALJTA || name == BUILTIN_SUTINAKA || name == BUILTIN_ULLANA_SIQI) {
            return "aru";
        }
        if (name == BUILTIN_CHANINAKA && !c->getArgs().empty()) {
            return mapValueType(c->getArgs()[0].get(), locals);
        }
        if ((name == BUILTIN_PUSH || name == BUILTIN_CHULLU ||
             name == BUILTIN_WAKICHA || name == BUILTIN_WAKICHA_QHIPA ||
             name == BUILTIN_SAPAKI || name == BUILTIN_KUTIKIPAT ||
             name == BUILTIN_JALJAT || name == BUILTIN_MAP ||
             name == BUILTIN_FILTER || name == BUILTIN_MAYJTAYA ||
             name == BUILTIN_AJLLI) && !c->getArgs().empty()) {
            return listElementType(c->getArgs()[0].get(), locals);
        }
        auto it = functionReturnTypes.find(c->getName());
        if (it == functionReturnTypes.end()) it = functionReturnTypes.find(name);
        if (it != functionReturnTypes.end()) {
            if (it->second.rfind("t'aqa:", 0) == 0) {
                return it->second.substr(6);
            }
            if (it->second == "t'aqa") {
                return "jakhüwi";
            }
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
        bool seenList = false;
        std::string listValType;
        for (const auto &item : map->getItems()) {
            if (isListExpr(item.second.get(), locals)) {
                seenList = true;
                if (listValType.empty()) {
                    std::string el = listElementType(item.second.get(), locals);
                    listValType = el.empty() ? "t'aqa:jakhüwi" : "t'aqa:" + el;
                }
            } else if (isStringExpr(item.second.get(), locals)) {
                seenString = true;
            } else {
                seenNumber = true;
            }
        }
        if (seenList && !seenString && !seenNumber) return listValType;
        if (seenString && !seenNumber && !seenList) return "aru";
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
    if (auto *m = dynamic_cast<const MemberExpr*>(expr)) {
        if (m->getResolvedType().rfind("mapa:", 0) == 0) {
            return m->getResolvedType().substr(5);
        }
        if (m->getResolvedType() == "mapa") {
            return "jakhüwi";
        }
    }
    if (auto *m = dynamic_cast<const MemberCallExpr*>(expr)) {
        if (m->getResolvedType().rfind("mapa:", 0) == 0) {
            return m->getResolvedType().substr(5);
        }
        if (m->getResolvedType() == "mapa") {
            return "jakhüwi";
        }
    }
    if (auto *i = dynamic_cast<const IndexExpr*>(expr)) {
        std::string inner = isMapExpr(i->getBase(), locals)
            ? mapValueType(i->getBase(), locals)
            : listElementType(i->getBase(), locals);
        if (inner.rfind("mapa:", 0) == 0) {
            return inner.substr(5);
        }
        if (inner == "mapa") {
            return "jakhüwi";
        }
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
            nameLower == BUILTIN_UTJI_SUTI || nameLower == BUILTIN_QALLTATI ||
            nameLower == BUILTIN_TUKUYATI || nameLower == BUILTIN_PICHHA_ARKATA) return true;
        auto it = functionReturnTypes.find(c->getName());
        if (it == functionReturnTypes.end()) it = functionReturnTypes.find(nameLower);
        if (it != functionReturnTypes.end() && it->second == "chiqa") return true;
    }
    if (auto *m = dynamic_cast<const MemberExpr*>(expr)) {
        if (!m->getResolvedType().empty()) return m->getResolvedType() == "chiqa";
    }
    if (auto *m = dynamic_cast<const MemberCallExpr*>(expr)) {
        if (!m->getResolvedType().empty()) return m->getResolvedType() == "chiqa";
    }
    if (auto *i = dynamic_cast<const IndexExpr*>(expr)) {
        if (isMapExpr(i->getBase(), locals)) {
            return mapValueType(i->getBase(), locals) == "chiqa";
        }
        if (!isStringExpr(i->getBase(), locals)) {
            return listElementType(i->getBase(), locals) == "chiqa";
        }
    }
    if (auto *v = dynamic_cast<const VariableExpr*>(expr)) {
        if (currentParamTypes.count(v->getName()) && currentParamTypes.at(v->getName()) == "chiqa")
            return true;
        if (currentLocalTypes.count(v->getName()) &&
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
