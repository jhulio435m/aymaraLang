#include "semantic.h"
#include <iostream>

namespace aym {

void SemanticAnalyzer::pushScope() {
    scopes.emplace_back();
}

void SemanticAnalyzer::popScope() {
    if (!scopes.empty()) scopes.pop_back();
}

void SemanticAnalyzer::declare(const std::string &name, const std::string &type) {
    if (scopes.empty()) pushScope();
    scopes.back()[name] = type;
}

bool SemanticAnalyzer::isDeclared(const std::string &name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->count(name)) return true;
    }
    return false;
}

std::string SemanticAnalyzer::lookup(const std::string &name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return f->second;
    }
    return "";
}

void SemanticAnalyzer::analyze(const std::vector<std::unique_ptr<Node>> &nodes) {
    pushScope();
    functions["willt’aña"] = 1; // built-in print function
    for (const auto &n : nodes) {
        analyzeStmt(static_cast<const Stmt *>(n.get()));
    }
    globals.clear();
    if (!scopes.empty()) {
        for (const auto &p : scopes.front()) globals.insert(p.first);
    }
    popScope();
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
        if (!isDeclared(a->getName())) {
            declare(a->getName(), t);
        } else if (lookup(a->getName()) != t && !t.empty()) {
            std::cerr << "Error: tipo incompatible en asignacion a '" << a->getName() << "'" << std::endl;
        }
        return;
    }
    if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt)) {
        std::string t = "";
        if (v->getInit()) t = analyzeExpr(v->getInit());
        declare(v->getName(), v->getType());
        if (!t.empty() && t != v->getType()) {
            std::cerr << "Error: tipo incompatible en declaracion de '" << v->getName() << "'" << std::endl;
        }
        return;
    }
    if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
        pushScope();
        for (const auto &s : b->statements) analyzeStmt(s.get());
        popScope();
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
        ++loopDepth;
        analyzeStmt(w->getBody());
        --loopDepth;
        return;
    }
    if (auto *f = dynamic_cast<const ForStmt *>(stmt)) {

        pushScope();
        analyzeStmt(f->getInit());
        analyzeExpr(f->getCondition());
        analyzeStmt(f->getPost());
        analyzeStmt(f->getBody());
        popScope();
        return;
    }
    if (auto *fn = dynamic_cast<const FunctionStmt *>(stmt)) {
        pushScope();
        for (const auto &pname : fn->getParams()) declare(pname, "int");
        analyzeStmt(fn->getBody());
        popScope();
        --loopDepth;
        return;
    }
    if (auto *fn = dynamic_cast<const FunctionStmt *>(stmt)) {
        functions[fn->getName()] = fn->getParams().size();
        ++functionDepth;
        analyzeStmt(fn->getBody());
        --functionDepth;
        return;
    }
    if (dynamic_cast<const BreakStmt *>(stmt)) {
        if (loopDepth == 0) {
            std::cerr << "Error: 'break' fuera de un ciclo" << std::endl;
        }
        return;
    }
    if (dynamic_cast<const ContinueStmt *>(stmt)) {
        if (loopDepth == 0) {
            std::cerr << "Error: 'continue' fuera de un ciclo" << std::endl;
        }
        return;
    }
    if (auto *r = dynamic_cast<const ReturnStmt *>(stmt)) {
        if (functionDepth == 0) {
            std::cerr << "Error: 'return' fuera de una funcion" << std::endl;
        }
        if (r->getValue()) analyzeExpr(r->getValue());
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
        if (!isDeclared(v->getName())) {
            std::cerr << "Error: variable '" << v->getName() << "' no declarada" << std::endl;
            return "";
        }
        return lookup(v->getName());
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
        auto it = functions.find(c->getName());
        if (it == functions.end()) {
            std::cerr << "Error: funcion '" << c->getName() << "' no declarada" << std::endl;
        } else if (c->getArgs().size() != it->second) {
            std::cerr << "Error: numero incorrecto de argumentos en llamada a '" << c->getName() << "'" << std::endl;
        }
        for (const auto &arg : c->getArgs()) analyzeExpr(arg.get());
        return "int";
    }
    return "";
}

} // namespace aym
