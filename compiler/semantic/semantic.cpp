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
    if (auto *e = dynamic_cast<const ExprStmt *>(stmt)) {
        analyzeExpr(e->getExpr());
        return;
    }
    if (auto *a = dynamic_cast<const AssignStmt *>(stmt)) {
        std::string t = analyzeExpr(a->getValue());
        if (!symbols.count(a->getName())) {
            symbols[a->getName()] = t;
        } else if (symbols[a->getName()] != t) {
            std::cerr << "Error: tipo incompatible en asignacion a '" << a->getName() << "'" << std::endl;
        }
        return;
    }
    if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt)) {
        std::string t = "";
        if (v->getInit()) t = analyzeExpr(v->getInit());
        symbols[v->getName()] = v->getType();
        if (!t.empty() && t != v->getType()) {
            std::cerr << "Error: tipo incompatible en declaracion de '" << v->getName() << "'" << std::endl;
        }
        return;
    }
    if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
        for (const auto &s : b->statements) analyzeStmt(s.get());
        return;
    }
    if (auto *i = dynamic_cast<const IfStmt *>(stmt)) {
        analyzeExpr(i->getCondition());
        analyzeStmt(i->getThen());
        if (i->getElse()) analyzeStmt(i->getElse());
        return;
    }
    if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) {
        analyzeExpr(w->getCondition());
        analyzeStmt(w->getBody());
        return;
    }
    if (auto *f = dynamic_cast<const ForStmt *>(stmt)) {
        analyzeStmt(f->getInit());
        analyzeExpr(f->getCondition());
        analyzeStmt(f->getPost());
        analyzeStmt(f->getBody());
        return;
    }
    if (auto *fn = dynamic_cast<const FunctionStmt *>(stmt)) {
        analyzeStmt(fn->getBody());
        return;
    }
}

std::string SemanticAnalyzer::analyzeExpr(const Expr *expr) {
    if (auto *n = dynamic_cast<const NumberExpr *>(expr)) {
        return "int";
    }
    if (auto *s = dynamic_cast<const StringExpr *>(expr)) {
        return "string";
    }
    if (auto *v = dynamic_cast<const VariableExpr *>(expr)) {
        if (!symbols.count(v->getName())) {
            std::cerr << "Error: variable '" << v->getName() << "' no declarada" << std::endl;
            return "";
        }
        return symbols[v->getName()];
    }
    if (auto *b = dynamic_cast<const BinaryExpr *>(expr)) {
        std::string l = analyzeExpr(b->getLeft());
        std::string r = analyzeExpr(b->getRight());
        if (l != r) {
            std::cerr << "Error: tipos incompatibles en operacion" << std::endl;
        }
        return l;
    }
    if (auto *c = dynamic_cast<const CallExpr *>(expr)) {
        for (const auto &arg : c->getArgs()) analyzeExpr(arg.get());
        return "int";
    }
    return "";
}

} // namespace aym
