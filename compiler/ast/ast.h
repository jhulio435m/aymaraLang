#ifndef AYM_AST_H
#define AYM_AST_H

#include <string>
#include <vector>
#include <memory>

namespace aym {

class NumberExpr;
class BoolExpr;
class StringExpr;
class VariableExpr;
class BinaryExpr;
class UnaryExpr;
class TernaryExpr;
class IncDecExpr;
class CallExpr;
class ListExpr;
class IndexExpr;
class PrintStmt;
class ExprStmt;
class AssignStmt;
class IndexAssignStmt;
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
    virtual void visit(BoolExpr &) = 0;
    virtual void visit(StringExpr &) = 0;
    virtual void visit(VariableExpr &) = 0;
    virtual void visit(BinaryExpr &) = 0;
    virtual void visit(UnaryExpr &) = 0;
    virtual void visit(TernaryExpr &) = 0;
    virtual void visit(IncDecExpr &) = 0;
    virtual void visit(CallExpr &) = 0;
    virtual void visit(ListExpr &) = 0;
    virtual void visit(IndexExpr &) = 0;
    virtual void visit(PrintStmt &) = 0;
    virtual void visit(ExprStmt &) = 0;
    virtual void visit(AssignStmt &) = 0;
    virtual void visit(IndexAssignStmt &) = 0;
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
    explicit NumberExpr(long long v) : value(v) {}
    long long getValue() const { return value; }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    long long value;
};

class BoolExpr : public Expr {
public:
    explicit BoolExpr(bool v) : value(v) {}
    bool getValue() const { return value; }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    bool value;
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

class TernaryExpr : public Expr {
public:
    TernaryExpr(std::unique_ptr<Expr> cond,
                std::unique_ptr<Expr> thenExpr,
                std::unique_ptr<Expr> elseExpr)
        : condition(std::move(cond)),
          thenBranch(std::move(thenExpr)),
          elseBranch(std::move(elseExpr)) {}
    Expr *getCondition() const { return condition.get(); }
    Expr *getThen() const { return thenBranch.get(); }
    Expr *getElse() const { return elseBranch.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> thenBranch;
    std::unique_ptr<Expr> elseBranch;
};

class IncDecExpr : public Expr {
public:
    IncDecExpr(std::string n, bool increment, bool prefix)
        : name(std::move(n)), isIncrement(increment), isPrefix(prefix) {}
    const std::string &getName() const { return name; }
    bool increment() const { return isIncrement; }
    bool prefix() const { return isPrefix; }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::string name;
    bool isIncrement;
    bool isPrefix;
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

class ListExpr : public Expr {
public:
    explicit ListExpr(std::vector<std::unique_ptr<Expr>> values)
        : elements(std::move(values)) {}
    const std::vector<std::unique_ptr<Expr>> &getElements() const { return elements; }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::vector<std::unique_ptr<Expr>> elements;
};

class IndexExpr : public Expr {
public:
    IndexExpr(std::unique_ptr<Expr> baseExpr, std::unique_ptr<Expr> indexExpr)
        : base(std::move(baseExpr)), index(std::move(indexExpr)) {}
    Expr *getBase() const { return base.get(); }
    Expr *getIndex() const { return index.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::unique_ptr<Expr> base;
    std::unique_ptr<Expr> index;
};

class PrintStmt : public Stmt {
public:
    PrintStmt(std::vector<std::unique_ptr<Expr>> exprs,
              std::unique_ptr<Expr> sep = nullptr,
              std::unique_ptr<Expr> term = nullptr)
        : expressions(std::move(exprs)),
          separator(std::move(sep)),
          terminator(std::move(term)) {}
    const std::vector<std::unique_ptr<Expr>> &getExprs() const { return expressions; }
    Expr *getSeparator() const { return separator.get(); }
    Expr *getTerminator() const { return terminator.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::vector<std::unique_ptr<Expr>> expressions;
    std::unique_ptr<Expr> separator;
    std::unique_ptr<Expr> terminator;
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

class IndexAssignStmt : public Stmt {
public:
    IndexAssignStmt(std::unique_ptr<Expr> baseExpr,
                    std::unique_ptr<Expr> indexExpr,
                    std::unique_ptr<Expr> valueExpr)
        : base(std::move(baseExpr)),
          index(std::move(indexExpr)),
          value(std::move(valueExpr)) {}
    Expr *getBase() const { return base.get(); }
    Expr *getIndex() const { return index.get(); }
    Expr *getValue() const { return value.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::unique_ptr<Expr> base;
    std::unique_ptr<Expr> index;
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
    struct Param {
        std::string type;
        std::string name;
    };

    FunctionStmt(std::string n,
                 std::vector<Param> p,
                 std::string r,
                 std::unique_ptr<BlockStmt> b)
        : name(std::move(n)), params(std::move(p)), returnType(std::move(r)), body(std::move(b)) {}
    const std::string &getName() const { return name; }
    const std::vector<Param> &getParams() const { return params; }
    const std::string &getReturnType() const { return returnType; }
    BlockStmt *getBody() const { return body.get(); }
    void accept(ASTVisitor &v) override { v.visit(*this); }
private:
    std::string name;
    std::vector<Param> params;
    std::string returnType;
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
