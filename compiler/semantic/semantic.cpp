#include "semantic.h"
#include <iostream>

namespace aym {

void SemanticAnalyzer::enterScope() { scopes.push_back({}); }
void SemanticAnalyzer::exitScope() { if(!scopes.empty()) scopes.pop_back(); }

std::string SemanticAnalyzer::lookup(const std::string &name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return f->second;
    }
    return "";
}

void SemanticAnalyzer::analyze(const std::vector<std::unique_ptr<Node>> &nodes) {
    enterScope();
    for (const auto &n : nodes) {
        analyzeStmt(static_cast<const Stmt *>(n.get()));
    }
    exitScope();
}

void SemanticAnalyzer::analyzeStmt(const Stmt *stmt) {
    if (auto *p = dynamic_cast<const PrintStmt *>(stmt)) {
        analyzeExpr(p->getExpr());
        return;
    }
    if (auto *a = dynamic_cast<const AssignStmt *>(stmt)) {
        std::string t = analyzeExpr(a->getValue());
        std::string prev = lookup(a->getName());
        if (prev.empty()) {
            scopes.back()[a->getName()] = t;
        } else if (prev != t) {
            std::cerr << "Error: tipo incompatible en asignacion a '" << a->getName() << "'" << std::endl;
        }
        return;
    }
    if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt)) {
        std::string t = "";
        if (v->getInit()) t = analyzeExpr(v->getInit());
        scopes.back()[v->getName()] = v->getType();
        if (!t.empty() && t != v->getType()) {
            std::cerr << "Error: tipo incompatible en declaracion de '" << v->getName() << "'" << std::endl;
        }
        return;
    }
    if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
        enterScope();
        for (const auto &s : b->statements) analyzeStmt(s.get());
        exitScope();
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
        enterScope();
        analyzeStmt(f->getInit());
        analyzeExpr(f->getCondition());
        analyzeStmt(f->getPost());
        analyzeStmt(f->getBody());
        exitScope();
        return;
    }
    if (auto *fn = dynamic_cast<const FunctionStmt *>(stmt)) {
        functions[fn->getName()] = const_cast<FunctionStmt*>(fn);
        enterScope();
        for (const auto &pname : fn->getParams()) scopes.back()[pname] = "int";
        analyzeStmt(fn->getBody());
        exitScope();
        return;
    }
}

std::string SemanticAnalyzer::analyzeExpr(const Expr *expr) {
    if (dynamic_cast<const NumberExpr *>(expr)) {
        return "int";
    }
    if (dynamic_cast<const StringExpr *>(expr)) {
        return "string";
    }
    if (auto *v = dynamic_cast<const VariableExpr *>(expr)) {
        std::string t = lookup(v->getName());
        if (t.empty()) {
            std::cerr << "Error: variable '" << v->getName() << "' no declarada" << std::endl;
            return "";
        }
        return t;
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
