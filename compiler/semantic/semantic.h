#ifndef AYM_SEMANTIC_H
#define AYM_SEMANTIC_H

#include <memory>
#include <unordered_map>
#include <vector>
#include "../ast/ast.h"

namespace aym {

class SemanticAnalyzer {
public:
    void analyze(const std::vector<std::unique_ptr<Node>> &nodes);

private:
    std::vector<std::unordered_map<std::string, std::string>> scopes;
    std::unordered_map<std::string, FunctionStmt*> functions;
    void analyzeStmt(const Stmt *stmt);
    std::string analyzeExpr(const Expr *expr);
    void enterScope();
    void exitScope();
    std::string lookup(const std::string &name);
};

} // namespace aym

#endif // AYM_SEMANTIC_H
