#include "semantic.h"
#include "../builtins/builtins.h"
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
    for (const auto &p : getBuiltinFunctions()) {
        functions[p.first] = p.second.argCount;
        if (!p.second.paramTypes.empty()) {
            std::vector<std::string> types;
            for (auto t : p.second.paramTypes) types.push_back(typeName(t));
            paramTypes[p.first] = types;
        }
    }

    struct FuncCollector : ASTVisitor {
        SemanticAnalyzer *self;
        void visit(FunctionStmt &fn) override {
            self->functions[fn.getName()] = fn.getParams().size();
            self->paramTypes[fn.getName()] = std::vector<std::string>(fn.getParams().size(), "jach’a");
        }
        void visit(NumberExpr&) override {}
        void visit(StringExpr&) override {}
        void visit(VariableExpr&) override {}
        void visit(BinaryExpr&) override {}
        void visit(UnaryExpr&) override {}
        void visit(CallExpr&) override {}
        void visit(PrintStmt&) override {}
        void visit(ExprStmt&) override {}
        void visit(AssignStmt&) override {}
        void visit(BlockStmt&) override {}
        void visit(IfStmt&) override {}
        void visit(ForStmt&) override {}
        void visit(BreakStmt&) override {}
        void visit(ContinueStmt&) override {}
        void visit(ReturnStmt&) override {}
        void visit(VarDeclStmt&) override {}
        void visit(WhileStmt&) override {}
        void visit(DoWhileStmt&) override {}
        void visit(SwitchStmt&) override {}
    } collector;
    collector.self = this;
    for (const auto &n : nodes) n->accept(collector);

    for (const auto &n : nodes) n->accept(*this);
    globals.clear();
    globalTypes.clear();
    if (!scopes.empty()) {
        for (const auto &p : scopes.front()) {
            globals.insert(p.first);
            globalTypes[p.first] = p.second;
        }
    }
    popScope();
}

void SemanticAnalyzer::visit(PrintStmt &p) {
    p.getExpr()->accept(*this);
}

void SemanticAnalyzer::visit(ExprStmt &e) {
    e.getExpr()->accept(*this);
}

void SemanticAnalyzer::visit(AssignStmt &a) {
    a.getValue()->accept(*this);
    std::string t = currentType;
    if (lastInputCall) t = lookup(a.getName());
    if (!isDeclared(a.getName())) {
        declare(a.getName(), t);
    } else if (lookup(a.getName()) != t && !t.empty()) {
        std::cerr << "Error: tipo incompatible en asignacion a '" << a.getName() << "'" << std::endl;
    }
}

void SemanticAnalyzer::visit(VarDeclStmt &v) {
    std::string t = "";
    if (v.getInit()) {
        v.getInit()->accept(*this);
        t = currentType;
        if (lastInputCall) t = v.getType();
    }
    declare(v.getName(), v.getType());
    if (!t.empty() && t != v.getType()) {
        std::cerr << "Error: tipo incompatible en declaracion de '" << v.getName() << "'" << std::endl;
    }
}

void SemanticAnalyzer::visit(BlockStmt &b) {
    pushScope();
    for (const auto &s : b.statements) s->accept(*this);
    popScope();
}

void SemanticAnalyzer::visit(IfStmt &i) {
    i.getCondition()->accept(*this);
    i.getThen()->accept(*this);
    if (i.getElse()) i.getElse()->accept(*this);
}

void SemanticAnalyzer::visit(WhileStmt &w) {
    w.getCondition()->accept(*this);
    ++loopDepth;
    w.getBody()->accept(*this);
    --loopDepth;
}

void SemanticAnalyzer::visit(DoWhileStmt &dw) {
    ++loopDepth;
    dw.getBody()->accept(*this);
    --loopDepth;
    dw.getCondition()->accept(*this);
}

void SemanticAnalyzer::visit(ForStmt &f) {
    pushScope();
    f.getInit()->accept(*this);
    f.getCondition()->accept(*this);
    f.getPost()->accept(*this);
    ++loopDepth;
    f.getBody()->accept(*this);
    --loopDepth;
    popScope();
}

void SemanticAnalyzer::visit(SwitchStmt &sw) {
    sw.getExpr()->accept(*this);
    ++switchDepth;
    for (const auto &c : sw.getCases()) {
        c.first->accept(*this);
        c.second->accept(*this);
    }
    if (sw.getDefault()) sw.getDefault()->accept(*this);
    --switchDepth;
}

void SemanticAnalyzer::visit(FunctionStmt &fn) {
    pushScope();
    ++functionDepth;
    auto it = paramTypes.find(fn.getName());
    size_t idx = 0;
    for (const auto &pname : fn.getParams()) {
        std::string t = "jach’a";
        if (it != paramTypes.end() && idx < it->second.size()) t = it->second[idx];
        declare(pname, t);
        ++idx;
    }
    fn.getBody()->accept(*this);
    --functionDepth;
    popScope();
}

void SemanticAnalyzer::visit(BreakStmt &) {
    if (loopDepth == 0 && switchDepth == 0) {
        std::cerr << "Error: 'break' fuera de un ciclo o switch" << std::endl;
    }
}

void SemanticAnalyzer::visit(ContinueStmt &) {
    if (loopDepth == 0) {
        std::cerr << "Error: 'continue' fuera de un ciclo" << std::endl;
    }
}

void SemanticAnalyzer::visit(ReturnStmt &r) {
    if (functionDepth == 0) {
        std::cerr << "Error: 'return' fuera de una funcion" << std::endl;
    }
    if (r.getValue()) r.getValue()->accept(*this);
}

void SemanticAnalyzer::visit(NumberExpr &) {
    currentType = "jach’a";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(StringExpr &) {
    currentType = "qillqa";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(VariableExpr &v) {
    if (!isDeclared(v.getName())) {
        std::cerr << "Error: variable '" << v.getName() << "' no declarada" << std::endl;
        currentType = "";
    } else {
        currentType = lookup(v.getName());
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(BinaryExpr &b) {
    b.getLeft()->accept(*this);
    std::string l = currentType;
    b.getRight()->accept(*this);
    std::string r = currentType;
    if (l != r) {
        std::cerr << "Error: tipos incompatibles en operacion" << std::endl;
    }
    char op = b.getOp();
    if (op=='&' || op=='|' || op=='s' || op=='d' || op=='<' || op=='>' || op=='l' || op=='g')
        currentType = "jach’a";
    else
        currentType = l;
    lastInputCall = false;
}

void SemanticAnalyzer::visit(UnaryExpr &u) {
    u.getExpr()->accept(*this);
    if (u.getOp() == '!') {
        currentType = "jach’a";
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(CallExpr &c) {
    auto it = functions.find(c.getName());
    if (it == functions.end()) {
        std::cerr << "Error: funcion '" << c.getName() << "' no declarada" << std::endl;
    } else if (c.getArgs().size() != it->second) {
        std::cerr << "Error: numero incorrecto de argumentos en llamada a '" << c.getName() << "'" << std::endl;
    }
    size_t idx = 0;
    auto pit = paramTypes.find(c.getName());
    for (const auto &arg : c.getArgs()) {
        arg->accept(*this);
        std::string t = currentType;
        if (pit != paramTypes.end() && idx < pit->second.size()) {
            if (t == "qillqa") pit->second[idx] = "qillqa";
        }
        ++idx;
    }
    currentType = "jach’a";
    lastInputCall = (c.getName() == BUILTIN_INPUT);
}

} // namespace aym
