#include "codegen_impl.h"
#include <algorithm>
#include <string>

namespace aym {

void CodeGenImpl::collectGlobal(const Stmt *stmt) {
    if (auto *v = dynamic_cast<const VarDeclStmt*>(stmt)) {
        globals.insert(v->getName());
        collectStrings(v->getInit());
        return;
    }
    if (auto *a = dynamic_cast<const AssignStmt*>(stmt)) {
        globals.insert(a->getName());
        collectStrings(a->getValue());
        return;
    }
    if (auto *a = dynamic_cast<const IndexAssignStmt*>(stmt)) {
        collectStrings(a->getBase());
        collectStrings(a->getIndex());
        collectStrings(a->getValue());
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
    if (auto *b = dynamic_cast<const BlockStmt*>(stmt)) {
        for (const auto &s : b->statements) collectGlobal(s.get());
        return;
    }
    if (auto *i = dynamic_cast<const IfStmt*>(stmt)) {
        collectStrings(i->getCondition());
        collectGlobal(i->getThen());
        if (i->getElse()) collectGlobal(i->getElse());
        return;
    }
    if (auto *w = dynamic_cast<const WhileStmt*>(stmt)) {
        collectStrings(w->getCondition());
        collectGlobal(w->getBody());
        return;
    }
    if (auto *dw = dynamic_cast<const DoWhileStmt*>(stmt)) {
        collectGlobal(dw->getBody());
        collectStrings(dw->getCondition());
        return;
    }
    if (auto *f = dynamic_cast<const ForStmt*>(stmt)) {
        collectGlobal(f->getInit());
        collectStrings(f->getCondition());
        collectGlobal(f->getPost());
        collectGlobal(f->getBody());
        return;
    }
    if (auto *sw = dynamic_cast<const SwitchStmt*>(stmt)) {
        collectStrings(sw->getExpr());
        for (const auto &c : sw->getCases()) {
            collectStrings(c.first.get());
            collectGlobal(c.second.get());
        }
        if (sw->getDefault()) collectGlobal(sw->getDefault());
        return;
    }
    if (auto *t = dynamic_cast<const TryStmt*>(stmt)) {
        if (!t->getHandlerSlot().empty()) {
            globals.insert(t->getHandlerSlot());
        }
        if (!t->getExceptionSlot().empty()) {
            globals.insert(t->getExceptionSlot());
        }
        collectGlobal(t->getTryBlock());
        for (const auto &c : t->getCatches()) {
            if (!c.varName.empty()) {
                globals.insert(c.varName);
            }
            collectGlobal(c.block.get());
            if (!c.typeName.empty()) {
                if (std::find(strings.begin(), strings.end(), c.typeName) == strings.end()) {
                    strings.push_back(c.typeName);
                }
            }
        }
        if (t->getFinallyBlock()) collectGlobal(t->getFinallyBlock());
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

void CodeGenImpl::assignTrySlots(Stmt *stmt) {
    if (!stmt) return;
    if (auto *b = dynamic_cast<BlockStmt*>(stmt)) {
        for (const auto &s : b->statements) assignTrySlots(s.get());
        return;
    }
    if (auto *i = dynamic_cast<IfStmt*>(stmt)) {
        assignTrySlots(i->getThen());
        if (i->getElse()) assignTrySlots(i->getElse());
        return;
    }
    if (auto *w = dynamic_cast<WhileStmt*>(stmt)) {
        assignTrySlots(w->getBody());
        return;
    }
    if (auto *dw = dynamic_cast<DoWhileStmt*>(stmt)) {
        assignTrySlots(dw->getBody());
        return;
    }
    if (auto *f = dynamic_cast<ForStmt*>(stmt)) {
        assignTrySlots(f->getInit());
        assignTrySlots(f->getPost());
        assignTrySlots(f->getBody());
        return;
    }
    if (auto *sw = dynamic_cast<SwitchStmt*>(stmt)) {
        for (const auto &c : sw->getCases()) {
            assignTrySlots(c.second.get());
        }
        if (sw->getDefault()) assignTrySlots(sw->getDefault());
        return;
    }
    if (auto *t = dynamic_cast<TryStmt*>(stmt)) {
        if (t->getHandlerSlot().empty()) {
            std::string suffix = std::to_string(tryTempCounter++);
            t->setHandlerSlot("__try_handler_" + suffix);
            t->setExceptionSlot("__try_exception_" + suffix);
        }
        assignTrySlots(t->getTryBlock());
        for (const auto &c : t->getCatches()) {
            assignTrySlots(c.block.get());
        }
        if (t->getFinallyBlock()) assignTrySlots(t->getFinallyBlock());
        return;
    }
}

} // namespace aym
