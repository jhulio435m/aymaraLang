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
    std::unordered_map<std::string, bool> symbols;
    void analyzeStmt(const Stmt *stmt);
    void analyzeExpr(const Expr *expr);
};

} // namespace aym

#endif // AYM_SEMANTIC_H
