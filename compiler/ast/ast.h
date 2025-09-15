#ifndef AYM_AST_H
#define AYM_AST_H

#include <string>
#include <vector>
#include <memory>

namespace aym {

class NumberExpr;
class StringExpr;
class VariableExpr;
class BinaryExpr;
class UnaryExpr;
class CallExpr;
class PrintStmt;
class ExprStmt;
class AssignStmt;
class BlockStmt;
class IfStmt;
class ForStmt;
class BreakStmt;
class ContinueStmt;
class ReturnStmt;
class VarDeclStmt;
class FunctionStmt;
class WhileStmt;
class DoWhileStmt;
class SwitchStmt;
class ImportStmt;

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void visit(NumberExpr &) = 0;
    virtual void visit(StringExpr &) = 0;
    virtual void visit(VariableExpr &) = 0;
    virtual void visit(BinaryExpr &) = 0;
    virtual void visit(UnaryExpr &) = 0;
    virtual void visit(CallExpr &) = 0;
    virtual void visit(PrintStmt &) = 0;
    virtual void visit(ExprStmt &) = 0;
    virtual void visit(AssignStmt &) = 0;
    virtual void visit(BlockStmt &) = 0;
    virtual void visit(IfStmt &) = 0;
    virtual void visit(ForStmt &) = 0;
    virtual void visit(BreakStmt &) = 0;
    virtual void visit(ContinueStmt &) = 0;
    virtual void visit(ReturnStmt &) = 0;
    virtual void visit(VarDeclStmt &) = 0;
    virtual void visit(FunctionStmt &) = 0;
    virtual void visit(WhileStmt &) = 0;
    virtual void visit(DoWhileStmt &) = 0;
    virtual void visit(SwitchStmt &) = 0;
    virtual void visit(ImportStmt &) = 0;
};

class Node {
public:
    Node() = default;
    Node(size_t line, size_t column) : line(line), column(column) {}
    virtual ~Node() = default;

    size_t getLine() const { return line; }
    size_t getColumn() const { return column; }
    void setLocation(size_t l, size_t c) { line = l; column = c; }

    virtual void accept(ASTVisitor &) = 0;
private:
    size_t line = 0;
    size_t column = 0;
};

class Expr : public Node {
};

class Stmt : public Node {
};

class NumberExpr : public Expr {
public:
    explicit NumberExpr(long v) : value(v) {}
    long getValue() const { return value; }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    long value;
};

class StringExpr : public Expr {
public:
    explicit StringExpr(const std::string &v) : value(v) {}
    const std::string &getValue() const { return value; }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::string value;
};

class VariableExpr : public Expr {
public:
    explicit VariableExpr(const std::string &n) : name(n) {}
    const std::string &getName() const { return name; }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::string name;
};

class BinaryExpr : public Expr {
public:
    BinaryExpr(char op,
               std::unique_ptr<Expr> lhs,
               std::unique_ptr<Expr> rhs)
        : oper(op), left(std::move(lhs)), right(std::move(rhs)) {}
    char getOp() const { return oper; }
    Expr *getLeft() const { return left.get(); }
    Expr *getRight() const { return right.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    char oper;
    std::unique_ptr<Expr> left, right;
};

class UnaryExpr : public Expr {
public:
    UnaryExpr(char o, std::unique_ptr<Expr> e) : op(o), expr(std::move(e)) {}
    char getOp() const { return op; }
    Expr *getExpr() const { return expr.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    char op;
    std::unique_ptr<Expr> expr;
};

class CallExpr : public Expr {
public:
    CallExpr(std::string callee,
             std::vector<std::unique_ptr<Expr>> args)
        : name(std::move(callee)), arguments(std::move(args)) {}
    const std::string &getName() const { return name; }
    const std::vector<std::unique_ptr<Expr>> &getArgs() const { return arguments; }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::string name;
    std::vector<std::unique_ptr<Expr>> arguments;
};

class PrintStmt : public Stmt {
public:
    explicit PrintStmt(std::unique_ptr<Expr> expr)
        : expression(std::move(expr)) {}
    Expr *getExpr() const { return expression.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::unique_ptr<Expr> expression;
};

class ExprStmt : public Stmt {
public:
    explicit ExprStmt(std::unique_ptr<Expr> e)
        : expression(std::move(e)) {}
    Expr *getExpr() const { return expression.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::unique_ptr<Expr> expression;
};

class AssignStmt : public Stmt {
public:
    AssignStmt(std::string n, std::unique_ptr<Expr> v)
        : name(std::move(n)), value(std::move(v)) {}
    const std::string &getName() const { return name; }
    Expr *getValue() const { return value.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::string name;
    std::unique_ptr<Expr> value;
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
    void accept(ASTVisitor &v) override { v.visit(*this); }
};

class IfStmt : public Stmt {
public:
    IfStmt(std::unique_ptr<Expr> cond,
           std::unique_ptr<BlockStmt> thenBlk,
           std::unique_ptr<BlockStmt> elseBlk = nullptr)
        : condition(std::move(cond)), thenBlock(std::move(thenBlk)),
          elseBlock(std::move(elseBlk)) {}
    Expr *getCondition() const { return condition.get(); }
    BlockStmt *getThen() const { return thenBlock.get(); }
    BlockStmt *getElse() const { return elseBlock.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> thenBlock;
    std::unique_ptr<BlockStmt> elseBlock;
};

class ForStmt : public Stmt {
public:
    ForStmt(std::unique_ptr<Stmt> init,
            std::unique_ptr<Expr> cond,
            std::unique_ptr<Stmt> post,
            std::unique_ptr<BlockStmt> body)
        : initStmt(std::move(init)), condition(std::move(cond)),
          postStmt(std::move(post)), body(std::move(body)) {}
    Stmt *getInit() const { return initStmt.get(); }
    Expr *getCondition() const { return condition.get(); }
    Stmt *getPost() const { return postStmt.get(); }
    BlockStmt *getBody() const { return body.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::unique_ptr<Stmt> initStmt;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> postStmt;
    std::unique_ptr<BlockStmt> body;
};

class BreakStmt : public Stmt { public: void accept(ASTVisitor &v) override { v.visit(*this); } };
class ContinueStmt : public Stmt { public: void accept(ASTVisitor &v) override { v.visit(*this); } };

class ReturnStmt : public Stmt {
public:
    explicit ReturnStmt(std::unique_ptr<Expr> v) : value(std::move(v)) {}
    Expr *getValue() const { return value.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::unique_ptr<Expr> value;
};

class VarDeclStmt : public Stmt {
public:
    VarDeclStmt(std::string t, std::string n, std::unique_ptr<Expr> i)
        : type(std::move(t)), name(std::move(n)), init(std::move(i)) {}
    const std::string &getType() const { return type; }
    const std::string &getName() const { return name; }
    Expr *getInit() const { return init.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::string type;
    std::string name;
    std::unique_ptr<Expr> init;
};

class FunctionStmt : public Stmt {
public:
    FunctionStmt(std::string n,
                 std::vector<std::string> p,
                 std::unique_ptr<BlockStmt> b)
        : name(std::move(n)), params(std::move(p)), body(std::move(b)) {}
    const std::string &getName() const { return name; }
    const std::vector<std::string> &getParams() const { return params; }
    BlockStmt *getBody() const { return body.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<BlockStmt> body;
};

class WhileStmt : public Stmt {
public:
    WhileStmt(std::unique_ptr<Expr> cond,
              std::unique_ptr<BlockStmt> body)
        : condition(std::move(cond)), body(std::move(body)) {}
    Expr *getCondition() const { return condition.get(); }
    BlockStmt *getBody() const { return body.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> body;
};

class DoWhileStmt : public Stmt {
public:
    DoWhileStmt(std::unique_ptr<BlockStmt> b,
                std::unique_ptr<Expr> cond)
        : body(std::move(b)), condition(std::move(cond)) {}
    Expr *getCondition() const { return condition.get(); }
    BlockStmt *getBody() const { return body.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::unique_ptr<BlockStmt> body;
    std::unique_ptr<Expr> condition;
};

class SwitchStmt : public Stmt {
public:
    SwitchStmt(std::unique_ptr<Expr> e,
               std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> cs,
               std::unique_ptr<BlockStmt> defc)
        : expr(std::move(e)), cases(std::move(cs)), defaultCase(std::move(defc)) {}
    Expr *getExpr() const { return expr.get(); }
    const std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> &getCases() const { return cases; }
    BlockStmt *getDefault() const { return defaultCase.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::unique_ptr<Expr> expr;
    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> cases;
    std::unique_ptr<BlockStmt> defaultCase;
};

class ImportStmt : public Stmt {
public:
    explicit ImportStmt(std::string module)
        : moduleName(std::move(module)) {}
    const std::string &getModule() const { return moduleName; }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::string moduleName;
};


} // namespace aym

#endif // AYM_AST_H
