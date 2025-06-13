#ifndef AYM_SEMANTIC_H
#define AYM_SEMANTIC_H

#include <memory>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include "../ast/ast.h"

namespace aym {

class SemanticAnalyzer {
public:
    void analyze(const std::vector<std::unique_ptr<Node>> &nodes);
    const std::unordered_set<std::string> &getGlobals() const { return globals; }

private:
    std::vector<std::unordered_map<std::string, std::string>> scopes;
    std::unordered_set<std::string> globals;

    void pushScope();
    void popScope();
    void declare(const std::string &name, const std::string &type);
    bool isDeclared(const std::string &name) const;
    std::string lookup(const std::string &name) const;
    void analyzeStmt(const Stmt *stmt);
    std::string analyzeExpr(const Expr *expr);
};

} // namespace aym

#endif // AYM_SEMANTIC_H
