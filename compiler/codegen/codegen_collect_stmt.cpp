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
                if (!expr) return "t'aqa:jakhüwi";
                if (auto *list = dynamic_cast<const ListExpr*>(expr)) {
                    bool allStrings = !list->getElements().empty();
                    for (const auto &elem : list->getElements()) {
                        if (!dynamic_cast<const StringExpr*>(elem.get())) {
                            allStrings = false;
                            break;
                        }
                    }
                    return allStrings ? "t'aqa:aru" : "t'aqa:jakhüwi";
                }
                if (auto *var = dynamic_cast<const VariableExpr*>(expr)) {
                    auto it = types.find(var->getName());
                    if (it != types.end() && it->second.rfind("t'aqa:", 0) == 0) {
                        return it->second;
                    }
                    return "t'aqa:jakhüwi";
                }
                if (auto *call = dynamic_cast<const CallExpr*>(expr)) {
                    std::string callName = lowerName(call->getName());
                    if ((callName == BUILTIN_WAKICHA || callName == BUILTIN_SAPAKI ||
                         callName == BUILTIN_MAP || callName == BUILTIN_FILTER ||
                         callName == BUILTIN_MAYJTAYA || callName == BUILTIN_AJLLI) &&
                        !call->getArgs().empty()) {
                        return self(call->getArgs()[0].get(), self);
                    }
                }
                return "t'aqa:jakhüwi";
            };
            declaredType = inferListType(v->getInit(), inferListType);
        }
        if (declaredType == "mapa") {
            if (auto *map = dynamic_cast<const MapExpr*>(v->getInit())) {
                bool seenString = false;
                bool seenNumber = false;
                for (const auto &item : map->getItems()) {
                    if (dynamic_cast<const StringExpr*>(item.second.get())) {
                        seenString = true;
                    } else {
                        seenNumber = true;
                    }
                }
                if (seenString && !seenNumber) {
                    declaredType = "mapa:aru";
                } else if (seenString && seenNumber) {
                    declaredType = "mapa";
                } else {
                    declaredType = "mapa:jakhüwi";
                }
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
                if (std::find(strings.begin(), strings.end(), c.typeName) == strings.end()) {
                    strings.push_back(c.typeName);
                }
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
            if (std::find(strings.begin(), strings.end(), "Error") == strings.end()) {
                strings.push_back("Error");
            }
        }
        return;
    }
}

} // namespace aym
