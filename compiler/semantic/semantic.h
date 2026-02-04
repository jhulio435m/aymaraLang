#ifndef AYM_SEMANTIC_H
#define AYM_SEMANTIC_H

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../ast/ast.h"

namespace aym {

class SemanticAnalyzer : public ASTVisitor {
public:
    void analyze(const std::vector<std::unique_ptr<Node>> &nodes);
    const std::unordered_set<std::string> &getGlobals() const { return globals; }
    const std::unordered_map<std::string, std::vector<std::string>> &getParamTypes() const { return paramTypes; }
    const std::unordered_map<std::string, std::string> &getGlobalTypes() const { return globalTypes; }
    const std::unordered_map<std::string, std::string> &getFunctionReturnTypes() const { return functionReturnTypes; }

private:
    std::vector<std::unordered_map<std::string, std::string>> scopes;
    std::unordered_map<std::string, size_t> functions;
    std::unordered_map<std::string, std::vector<std::string>> paramTypes;
    std::unordered_map<std::string, std::string> functionReturnTypes;
    std::unordered_set<std::string> globals;
    std::unordered_map<std::string, std::string> globalTypes;
    int loopDepth = 0;
    int switchDepth = 0;
    int functionDepth = 0;

    void pushScope();
    void popScope();
    void declare(const std::string &name, const std::string &type);
    bool isDeclared(const std::string &name) const;
    std::string lookup(const std::string &name) const;

    std::string currentType;
    bool lastInputCall = false;

    void visit(NumberExpr &) override;
    void visit(BoolExpr &) override;
    void visit(StringExpr &) override;
    void visit(VariableExpr &) override;
    void visit(BinaryExpr &) override;
    void visit(UnaryExpr &) override;
    void visit(TernaryExpr &) override;
    void visit(IncDecExpr &) override;
    void visit(CallExpr &) override;
    void visit(ListExpr &) override;
    void visit(IndexExpr &) override;
    void visit(MemberExpr &) override;
    void visit(PrintStmt &) override;
    void visit(ExprStmt &) override;
    void visit(AssignStmt &) override;
    void visit(IndexAssignStmt &) override;
    void visit(BlockStmt &) override;
    void visit(IfStmt &) override;
    void visit(ForStmt &) override;
    void visit(BreakStmt &) override;
    void visit(ContinueStmt &) override;
    void visit(ReturnStmt &) override;
    void visit(VarDeclStmt &) override;
    void visit(FunctionStmt &) override;
    void visit(WhileStmt &) override;
    void visit(DoWhileStmt &) override;
    void visit(SwitchStmt &) override;
    void visit(ImportStmt &) override;
    void visit(ThrowStmt &) override;
    void visit(TryStmt &) override;
}; 

} // namespace aym

#endif // AYM_SEMANTIC_H
