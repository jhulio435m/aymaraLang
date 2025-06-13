#ifndef AYM_AST_H
#define AYM_AST_H

#include <string>
#include <vector>
#include <memory>

namespace aym {

class Node {
public:
    virtual ~Node() = default;
};

class Expr : public Node {
};

class Stmt : public Node {
};

class NumberExpr : public Expr {
public:
    explicit NumberExpr(long v) : value(v) {}
    long getValue() const { return value; }
private:
    long value;
};

class StringExpr : public Expr {
public:
    explicit StringExpr(const std::string &v) : value(v) {}
    const std::string &getValue() const { return value; }
private:
    std::string value;
};

class VariableExpr : public Expr {
public:
    explicit VariableExpr(const std::string &n) : name(n) {}
    const std::string &getName() const { return name; }
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
private:
    char oper;
    std::unique_ptr<Expr> left, right;
};

class CallExpr : public Expr {
public:
    CallExpr(std::string callee,
             std::vector<std::unique_ptr<Expr>> args)
        : name(std::move(callee)), arguments(std::move(args)) {}
    const std::string &getName() const { return name; }
    const std::vector<std::unique_ptr<Expr>> &getArgs() const { return arguments; }
private:
    std::string name;
    std::vector<std::unique_ptr<Expr>> arguments;
};

class PrintStmt : public Stmt {
public:
    explicit PrintStmt(std::unique_ptr<Expr> expr)
        : expression(std::move(expr)) {}
    Expr *getExpr() const { return expression.get(); }
private:
    std::unique_ptr<Expr> expression;
};

class AssignStmt : public Stmt {
public:
    AssignStmt(std::string n, std::unique_ptr<Expr> v)
        : name(std::move(n)), value(std::move(v)) {}
    const std::string &getName() const { return name; }
    Expr *getValue() const { return value.get(); }
private:
    std::string name;
    std::unique_ptr<Expr> value;
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
};

class IfStmt : public Stmt {
public:
    IfStmt(std::unique_ptr<Expr> cond,
           std::unique_ptr<BlockStmt> body)
        : condition(std::move(cond)), thenBlock(std::move(body)) {}
    Expr *getCondition() const { return condition.get(); }
    BlockStmt *getThen() const { return thenBlock.get(); }
private:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> thenBlock;
};

class WhileStmt : public Stmt {
public:
    WhileStmt(std::unique_ptr<Expr> cond,
              std::unique_ptr<BlockStmt> body)
        : condition(std::move(cond)), body(std::move(body)) {}
    Expr *getCondition() const { return condition.get(); }
    BlockStmt *getBody() const { return body.get(); }
private:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> body;
};


} // namespace aym

#endif // AYM_AST_H