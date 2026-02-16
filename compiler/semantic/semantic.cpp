#include "semantic.h"
#include "../builtins/builtins.h"
#include "../utils/class_names.h"
#include "../utils/diagnostic.h"
#include <iostream>
#include <algorithm>

namespace aym {

void SemanticAnalyzer::markNode(const Node &node) {
    currentLine = node.getLine();
    currentColumn = node.getColumn();
}

void SemanticAnalyzer::reportError(const std::string &message, const std::string &code) {
    hadErrors = true;
    if (diagnostics) {
        diagnostics->error(code, message, currentLine, currentColumn);
        return;
    }
    if (currentLine > 0) {
        std::cerr << "Error en linea " << currentLine << ", columna " << currentColumn
                  << ": " << message << std::endl;
        return;
    }
    std::cerr << "Error: " << message << std::endl;
}

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
    hadErrors = false;
    scopes.clear();
    globals.clear();
    globalTypes.clear();
    functions.clear();
    paramTypes.clear();
    functionReturnTypes.clear();
    classes.clear();
    currentClass.clear();
    currentBaseClass.clear();
    loopDepth = 0;
    switchDepth = 0;
    functionDepth = 0;
    currentType.clear();
    lastInputCall = false;
    currentLine = 0;
    currentColumn = 0;

    pushScope();
    for (const auto &p : getBuiltinFunctions()) {
        functions[p.first] = p.second.argCount;
        if (!p.second.paramTypes.empty()) {
            std::vector<std::string> types;
            for (auto t : p.second.paramTypes) types.push_back(typeName(t));
            paramTypes[p.first] = types;
        }
    }
    collectClassInfo(nodes);

    struct FuncCollector : ASTVisitor {
        SemanticAnalyzer *self;
        void visit(FunctionStmt &fn) override {
            self->functions[fn.getName()] = fn.getParams().size();
            std::vector<std::string> types;
            for (const auto &p : fn.getParams()) {
                std::string t = p.type;
                if (t == "t'aqa") t = "t'aqa:jakhüwi";
                if (t == "mapa") t = "mapa:jakhüwi";
                if (self->isClassName(t)) t = "kasta:" + t;
                types.push_back(t);
            }
            self->paramTypes[fn.getName()] = std::move(types);
            if (!fn.getReturnType().empty()) {
                std::string ret = fn.getReturnType();
                if (ret == "t'aqa") ret = "t'aqa:jakhüwi";
                if (ret == "mapa") ret = "mapa:jakhüwi";
                if (self->isClassName(ret)) ret = "kasta:" + ret;
                self->functionReturnTypes[fn.getName()] = ret;
            }
        }
        void visit(NumberExpr&) override {}
        void visit(BoolExpr&) override {}
        void visit(StringExpr&) override {}
        void visit(VariableExpr&) override {}
        void visit(BinaryExpr&) override {}
        void visit(UnaryExpr&) override {}
        void visit(TernaryExpr&) override {}
        void visit(IncDecExpr&) override {}
        void visit(CallExpr&) override {}
        void visit(ListExpr&) override {}
        void visit(MapExpr&) override {}
        void visit(IndexExpr&) override {}
        void visit(MemberExpr&) override {}
        void visit(PrintStmt&) override {}
        void visit(ExprStmt&) override {}
        void visit(AssignStmt&) override {}
        void visit(IndexAssignStmt&) override {}
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
        void visit(ImportStmt&) override {}
        void visit(ThrowStmt&) override {}
        void visit(TryStmt&) override {}
        void visit(ClassStmt&) override {}
        void visit(MemberCallExpr&) override {}
        void visit(NewExpr&) override {}
        void visit(FunctionRefExpr&) override {}
        void visit(SuperExpr&) override {}
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

} // namespace aym
