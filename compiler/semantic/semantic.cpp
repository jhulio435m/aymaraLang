#include "semantic.h"
#include <iostream>

namespace aym {

void SemanticAnalyzer::analyze(const std::vector<std::unique_ptr<Node>> &nodes) {
    for (const auto &n : nodes) {
        analyzeStmt(static_cast<const Stmt *>(n.get()));
    }
}

void SemanticAnalyzer::analyzeStmt(const Stmt *stmt) {
    if (auto *p = dynamic_cast<const PrintStmt *>(stmt)) {
        analyzeExpr(p->getExpr());
        return;
    }
    if (auto *a = dynamic_cast<const AssignStmt *>(stmt)) {
        analyzeExpr(a->getValue());
        symbols[a->getName()] = true;
        return;
    }
    if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
        for (const auto &s : b->statements) analyzeStmt(s.get());
        return;
    }
    if (auto *i = dynamic_cast<const IfStmt *>(stmt)) {
        analyzeExpr(i->getCondition());
        analyzeStmt(i->getThen());
        return;
    }
    if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) {
        analyzeExpr(w->getCondition());
        analyzeStmt(w->getBody());
        return;
    }
}

void SemanticAnalyzer::analyzeExpr(const Expr *expr) {
    if (auto *n = dynamic_cast<const NumberExpr *>(expr)) {
        (void)n; // nothing to do
        return;
    }
    if (auto *s = dynamic_cast<const StringExpr *>(expr)) {
        (void)s;
        return;
    }
    if (auto *v = dynamic_cast<const VariableExpr *>(expr)) {
        if (!symbols.count(v->getName())) {
            std::cerr << "Error: variable '" << v->getName() << "' no declarada" << std::endl;
        }
        return;
    }
    if (auto *b = dynamic_cast<const BinaryExpr *>(expr)) {
        analyzeExpr(b->getLeft());
        analyzeExpr(b->getRight());
        return;
    }
    if (auto *c = dynamic_cast<const CallExpr *>(expr)) {
        for (const auto &arg : c->getArgs()) analyzeExpr(arg.get());
        return;
    }
}

} // namespace aym
