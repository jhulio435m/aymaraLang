#ifndef AYM_INTERPRETER_H
#define AYM_INTERPRETER_H

#include "../ast/ast.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

namespace aym {

struct Value {
    enum class Type { Int, Float, Bool, String, Void } type = Type::Void;
    long i = 0;
    double f = 0.0;
    bool b = false;
    std::string s;

    static Value Int(long v) { Value val; val.type = Type::Int; val.i = v; return val; }
    static Value Float(double v) { Value val; val.type = Type::Float; val.f = v; return val; }
    static Value Bool(bool v) { Value val; val.type = Type::Bool; val.b = v; return val; }
    static Value String(std::string v) { Value val; val.type = Type::String; val.s = std::move(v); return val; }
    static Value Void() { return Value(); }
};

class Interpreter : public ASTVisitor {
public:
    Interpreter();
    explicit Interpreter(unsigned int seed);
    void setSeed(unsigned int seed);
    void execute(const std::vector<std::unique_ptr<Node>> &nodes);
    Value getLastValue() const { return lastValue; }

    // ASTVisitor overrides
    void visit(NumberExpr&) override;
    void visit(StringExpr&) override;
    void visit(VariableExpr&) override;
    void visit(BinaryExpr&) override;
    void visit(UnaryExpr&) override;
    void visit(CallExpr&) override;
    void visit(PrintStmt&) override;
    void visit(ExprStmt&) override;
    void visit(AssignStmt&) override;
    void visit(BlockStmt&) override;
    void visit(IfStmt&) override;
    void visit(ForStmt&) override;
    void visit(BreakStmt&) override;
    void visit(ContinueStmt&) override;
    void visit(ReturnStmt&) override;
    void visit(VarDeclStmt&) override;
    void visit(FunctionStmt&) override;
    void visit(WhileStmt&) override;
    void visit(DoWhileStmt&) override;
    void visit(SwitchStmt&) override;

private:
    Value lastValue;
    bool randSeeded = false;
    bool breakFlag = false;
    bool continueFlag = false;
    bool returnFlag = false;
    Value returnValue;

    std::vector<std::unordered_map<std::string, Value>> scopes{1};
    std::unordered_map<std::string, FunctionStmt*> functions;
    std::vector<std::vector<long>> arrays;
    std::vector<bool> arraysValid;

    void pushScope();
    void popScope();
    Value lookup(const std::string &name, size_t line, size_t column);
    void assign(const std::string &name, const Value &val);
    void declare(const std::string &name, const Value &val);

    Value eval(Expr* expr);
    Value callFunction(const std::string &name, const std::vector<Value>& args, size_t line, size_t column);
};

} // namespace aym

#endif // AYM_INTERPRETER_H
