#include "codegen_impl.h"
#include "../utils/class_names.h"
#include <algorithm>
#include <string>

namespace aym {

void CodeGenImpl::collectStrings(const Expr *expr) {
    if (!expr) return;
    if (auto *s = dynamic_cast<const StringExpr*>(expr)) {
        if (std::find(strings.begin(), strings.end(), s->getValue()) == strings.end())
            strings.push_back(s->getValue());
        return;
    }
    if (auto *b = dynamic_cast<const BinaryExpr*>(expr)) {
        collectStrings(b->getLeft());
        collectStrings(b->getRight());
        return;
    }
    if (auto *u = dynamic_cast<const UnaryExpr*>(expr)) {
        collectStrings(u->getExpr());
        return;
    }
    if (auto *t = dynamic_cast<const TernaryExpr*>(expr)) {
        collectStrings(t->getCondition());
        collectStrings(t->getThen());
        collectStrings(t->getElse());
        return;
    }
    if (auto *l = dynamic_cast<const ListExpr*>(expr)) {
        for (const auto &elem : l->getElements()) {
            collectStrings(elem.get());
        }
        return;
    }
    if (auto *m = dynamic_cast<const MapExpr*>(expr)) {
        for (const auto &item : m->getItems()) {
            collectStrings(item.first.get());
            collectStrings(item.second.get());
        }
        return;
    }
    if (auto *i = dynamic_cast<const IndexExpr*>(expr)) {
        collectStrings(i->getBase());
        collectStrings(i->getIndex());
        return;
    }
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        for (const auto &a : c->getArgs()) collectStrings(a.get());
        return;
    }
    if (auto *m = dynamic_cast<const MemberCallExpr*>(expr)) {
        for (const auto &a : m->getArgs()) collectStrings(a.get());
        collectStrings(m->getBase());
        if (std::find(strings.begin(), strings.end(), m->getMember()) == strings.end()) {
            strings.push_back(m->getMember());
        }
        if (dynamic_cast<const SuperExpr*>(m->getBase())) {
            std::string superKey = classSuperKey(m->getMember());
            if (std::find(strings.begin(), strings.end(), superKey) == strings.end()) {
                strings.push_back(superKey);
            }
        }
        return;
    }
    if (auto *n = dynamic_cast<const NewExpr*>(expr)) {
        for (const auto &a : n->getArgs()) collectStrings(a.get());
        return;
    }
    if (auto *f = dynamic_cast<const FunctionRefExpr*>(expr)) {
        (void)f;
        return;
    }
    if (auto *s = dynamic_cast<const SuperExpr*>(expr)) {
        (void)s;
        return;
    }
    if (auto *m = dynamic_cast<const MemberExpr*>(expr)) {
        collectStrings(m->getBase());
        if (std::find(strings.begin(), strings.end(), m->getMember()) == strings.end()) {
            strings.push_back(m->getMember());
        }
        return;
    }
}

} // namespace aym
