#include "codegen_impl.h"
#include "../builtins/builtins.h"
#include <algorithm>
#include <string>

namespace aym {

void CodeGenImpl::collectLocals(const Stmt *stmt,
                                std::vector<std::string> &locs,
                                std::unordered_map<std::string,bool> &strs,
                                std::unordered_map<std::string,std::string> &types) {
    if (auto *v = dynamic_cast<const VarDeclStmt*>(stmt)) {
        locs.push_back(v->getName());
        if (v->getType() == "aru") strs[v->getName()] = true;
        std::string declaredType = v->getType();
        if (declaredType == "t'aqa") {
            auto inferListType = [&](const Expr *expr, auto &&self) -> std::string {
                if (!expr) return "";
                if (auto *list = dynamic_cast<const ListExpr*>(expr)) {
                    if (list->getElements().empty()) {
                        return "t'aqa:jakhüwi";
                    }
                    std::string nestedListType;
                    bool sawList = false;
                    for (const auto &elem : list->getElements()) {
                        std::string sub = self(elem.get(), self);
                        if (!sub.empty()) {
                            sawList = true;
                            if (auto *subList = dynamic_cast<const ListExpr*>(elem.get())) {
                                if (!subList->getElements().empty()) {
                                    nestedListType = sub;
                                    break;
                                }
                            }
                            if (nestedListType.empty()) {
                                nestedListType = sub;
                            }
                        }
                    }
                    if (sawList) {
                        return "t'aqa:" + (nestedListType.empty() ? std::string("t'aqa:jakhüwi") : nestedListType);
                    }
                    bool allStrings = true;
                    for (const auto &elem : list->getElements()) {
                        bool elemIsStr = isStringExpr(elem.get(), nullptr);
                        if (!elemIsStr) {
                            if (auto *var = dynamic_cast<const VariableExpr*>(elem.get())) {
                                auto sit = strs.find(var->getName());
                                if (sit != strs.end() && sit->second) {
                                    elemIsStr = true;
                                }
                            }
                        }
                        if (!elemIsStr) {
                            allStrings = false;
                            break;
                        }
                    }
                    if (allStrings) {
                        return "t'aqa:aru";
                    }
                    bool allMaps = true;
                    for (const auto &elem : list->getElements()) {
                        bool elemIsMap = isMapExpr(elem.get(), nullptr);
                        if (!elemIsMap) {
                            if (auto *var = dynamic_cast<const VariableExpr*>(elem.get())) {
                                auto tit = types.find(var->getName());
                                if (tit != types.end() &&
                                    (tit->second.rfind("mapa", 0) == 0 || tit->second.rfind("kasta:", 0) == 0)) {
                                    elemIsMap = true;
                                }
                            }
                        }
                        if (!elemIsMap) {
                            allMaps = false;
                            break;
                        }
                    }
                    if (allMaps) {
                        return "t'aqa:mapa";
                    }
                    return "t'aqa:jakhüwi";
                }
                if (auto *var = dynamic_cast<const VariableExpr*>(expr)) {
                    auto it = types.find(var->getName());
                    if (it != types.end()) {
                        if (it->second.rfind("t'aqa:", 0) == 0) return it->second;
                        if (it->second == "t'aqa") return "t'aqa:jakhüwi";
                    }
                    auto git = globalTypes.find(var->getName());
                    if (git != globalTypes.end()) {
                        if (git->second.rfind("t'aqa:", 0) == 0) return git->second;
                        if (git->second == "t'aqa") return "t'aqa:jakhüwi";
                    }
                    return "";
                }
                if (auto *m = dynamic_cast<const MemberExpr*>(expr)) {
                    if (m->getResolvedType().rfind("t'aqa:", 0) == 0) return m->getResolvedType();
                    if (m->getResolvedType() == "t'aqa") return "t'aqa:jakhüwi";
                    return "";
                }
                if (auto *mc = dynamic_cast<const MemberCallExpr*>(expr)) {
                    if (mc->getResolvedType().rfind("t'aqa:", 0) == 0) return mc->getResolvedType();
                    if (mc->getResolvedType() == "t'aqa") return "t'aqa:jakhüwi";
                    return "";
                }
                if (auto *idx = dynamic_cast<const IndexExpr*>(expr)) {
                    std::string baseT = self(idx->getBase(), self);
                    if (baseT.rfind("t'aqa:", 0) == 0) {
                        std::string inner = baseT.substr(6);
                        if (inner.rfind("t'aqa:", 0) == 0) return inner;
                        if (inner == "t'aqa") return "t'aqa:jakhüwi";
                    }
                    return "";
                }
                if (auto *call = dynamic_cast<const CallExpr*>(expr)) {
                    std::string callName = lowerName(call->getName());
                    if (callName == BUILTIN_ULLANA_SIQI || callName == BUILTIN_JALJTA ||
                        callName == BUILTIN_SUTINAKA) {
                        return "t'aqa:aru";
                    }
                    if ((callName == BUILTIN_WAKICHA || callName == BUILTIN_WAKICHA_QHIPA ||
                         callName == BUILTIN_SAPAKI || callName == BUILTIN_MAP ||
                         callName == BUILTIN_FILTER || callName == BUILTIN_MAYJTAYA ||
                         callName == BUILTIN_AJLLI || callName == BUILTIN_KUTIKIPAT ||
                         callName == BUILTIN_JALJAT || callName == BUILTIN_PUSH ||
                         callName == BUILTIN_CHULLU) &&
                        !call->getArgs().empty()) {
                        return self(call->getArgs()[0].get(), self);
                    }
                    if (callName == BUILTIN_CHANINAKA && !call->getArgs().empty()) {
                        std::string vt = mapValueType(call->getArgs()[0].get(), nullptr);
                        if (vt.empty()) {
                            if (auto *var = dynamic_cast<const VariableExpr*>(call->getArgs()[0].get())) {
                                auto tit = types.find(var->getName());
                                if (tit != types.end() && tit->second.rfind("mapa:", 0) == 0) {
                                    vt = tit->second.substr(5);
                                }
                            }
                        }
                        if (!vt.empty()) return "t'aqa:" + vt;
                        return "t'aqa:jakhüwi";
                    }
                    auto fit = functionReturnTypes.find(call->getName());
                    if (fit == functionReturnTypes.end()) fit = functionReturnTypes.find(callName);
                    if (fit != functionReturnTypes.end()) {
                        if (fit->second.rfind("t'aqa:", 0) == 0) return fit->second;
                        if (fit->second == "t'aqa") return "t'aqa:jakhüwi";
                    }
                    return "";
                }
                return "";
            };
            std::string inferred = inferListType(v->getInit(), inferListType);
            declaredType = inferred.empty() ? "t'aqa:jakhüwi" : inferred;
        }
        if (declaredType == "mapa") {
            if (auto *map = dynamic_cast<const MapExpr*>(v->getInit())) {
                bool seenString = false;
                bool seenNumber = false;
                bool seenList = false;
                std::string listValType;
                for (const auto &item : map->getItems()) {
                    if (dynamic_cast<const ListExpr*>(item.second.get()) ||
                        isListExpr(item.second.get(), nullptr)) {
                        seenList = true;
                        if (listValType.empty()) {
                            std::string el = listElementType(item.second.get(), nullptr);
                            listValType = el.empty() ? "t'aqa:jakhüwi" : "t'aqa:" + el;
                        }
                    } else if (isStringExpr(item.second.get(), nullptr) ||
                               (dynamic_cast<const VariableExpr*>(item.second.get()) &&
                                strs.count(static_cast<const VariableExpr*>(item.second.get())->getName()) &&
                                strs[static_cast<const VariableExpr*>(item.second.get())->getName()])) {
                        seenString = true;
                    } else {
                        seenNumber = true;
                    }
                }
                if (seenList && !seenString && !seenNumber) {
                    declaredType = "mapa:" + listValType;
                } else if (seenString && !seenNumber && !seenList) {
                    declaredType = "mapa:aru";
                } else if ((seenString && seenNumber) || (seenList && (seenString || seenNumber))) {
                    declaredType = "mapa";
                } else {
                    declaredType = "mapa:jakhüwi";
                }
            } else if (auto *var = dynamic_cast<const VariableExpr*>(v->getInit())) {
                if (types.count(var->getName()) && types[var->getName()].rfind("mapa", 0) == 0) {
                    declaredType = types[var->getName()];
                } else if (globalTypes.count(var->getName()) && globalTypes.at(var->getName()).rfind("mapa", 0) == 0) {
                    declaredType = globalTypes.at(var->getName());
                } else {
                    declaredType = "mapa:jakhüwi";
                }
            } else if (auto *m = dynamic_cast<const MemberExpr*>(v->getInit())) {
                declaredType = (m->getResolvedType().rfind("mapa", 0) == 0) ? m->getResolvedType() : "mapa:jakhüwi";
            } else if (auto *mc = dynamic_cast<const MemberCallExpr*>(v->getInit())) {
                declaredType = (mc->getResolvedType().rfind("mapa", 0) == 0) ? mc->getResolvedType() : "mapa:jakhüwi";
            } else {
                declaredType = "mapa:jakhüwi";
            }
        }
        types[v->getName()] = declaredType;
        collectStrings(v->getInit());
        return;
    }
    if (auto *p = dynamic_cast<const PrintStmt*>(stmt)) {
        for (const auto &expr : p->getExprs()) {
            collectStrings(expr.get());
        }
        collectStrings(p->getSeparator());
        collectStrings(p->getTerminator());
        return;
    }
    if (auto *a = dynamic_cast<const AssignStmt*>(stmt)) {
        collectStrings(a->getValue());
        return;
    }
    if (auto *a = dynamic_cast<const IndexAssignStmt*>(stmt)) {
        collectStrings(a->getBase());
        collectStrings(a->getIndex());
        collectStrings(a->getValue());
        return;
    }
    if (auto *b = dynamic_cast<const BlockStmt*>(stmt)) {
        for (const auto &s : b->statements) collectLocals(s.get(), locs, strs, types);
        return;
    }
    if (auto *i = dynamic_cast<const IfStmt*>(stmt)) {
        collectStrings(i->getCondition());
        collectLocals(i->getThen(), locs, strs, types);
        if (i->getElse()) collectLocals(i->getElse(), locs, strs, types);
        return;
    }
    if (auto *w = dynamic_cast<const WhileStmt*>(stmt)) {
        collectStrings(w->getCondition());
        collectLocals(w->getBody(), locs, strs, types);
        return;
    }
    if (auto *dw = dynamic_cast<const DoWhileStmt*>(stmt)) {
        collectLocals(dw->getBody(), locs, strs, types);
        collectStrings(dw->getCondition());
        return;
    }
    if (auto *f = dynamic_cast<const ForStmt*>(stmt)) {
        collectLocals(f->getInit(), locs, strs, types);
        collectStrings(f->getCondition());
        collectLocals(f->getPost(), locs, strs, types);
        collectLocals(f->getBody(), locs, strs, types);
        return;
    }
    if (auto *sw = dynamic_cast<const SwitchStmt*>(stmt)) {
        collectStrings(sw->getExpr());
        for (const auto &c : sw->getCases()) {
            collectStrings(c.first.get());
            collectLocals(c.second.get(), locs, strs, types);
        }
        if (sw->getDefault()) collectLocals(sw->getDefault(), locs, strs, types);
        return;
    }
    if (auto *t = dynamic_cast<const TryStmt*>(stmt)) {
        if (!t->getHandlerSlot().empty()) {
            if (std::find(locs.begin(), locs.end(), t->getHandlerSlot()) == locs.end()) {
                locs.push_back(t->getHandlerSlot());
                types[t->getHandlerSlot()] = "handler";
            }
        }
        if (!t->getExceptionSlot().empty()) {
            if (std::find(locs.begin(), locs.end(), t->getExceptionSlot()) == locs.end()) {
                locs.push_back(t->getExceptionSlot());
                types[t->getExceptionSlot()] = "excepcion";
            }
        }
        collectLocals(t->getTryBlock(), locs, strs, types);
        for (const auto &c : t->getCatches()) {
            if (!c.varName.empty()) {
                if (std::find(locs.begin(), locs.end(), c.varName) == locs.end()) {
                    locs.push_back(c.varName);
                    types[c.varName] = "excepcion";
                }
            }
            if (!c.typeName.empty()) {
                registerString(c.typeName);
            }
            collectLocals(c.block.get(), locs, strs, types);
        }
        if (t->getFinallyBlock()) collectLocals(t->getFinallyBlock(), locs, strs, types);
        return;
    }
    if (auto *ret = dynamic_cast<const ReturnStmt*>(stmt)) {
        collectStrings(ret->getValue());
        return;
    }
    if (auto *e = dynamic_cast<const ExprStmt*>(stmt)) {
        collectStrings(e->getExpr());
        return;
    }
    if (auto *thr = dynamic_cast<const ThrowStmt*>(stmt)) {
        collectStrings(thr->getType());
        collectStrings(thr->getMessage());
        if (!thr->getType()) {
            registerString("Error");
        }
        return;
    }
}

} // namespace aym
