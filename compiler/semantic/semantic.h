#ifndef AYM_SEMANTIC_H
#define AYM_SEMANTIC_H

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../ast/ast.h"

namespace aym {

class DiagnosticEngine;

class SemanticAnalyzer : public ASTVisitor {
public:
    void analyze(const std::vector<std::unique_ptr<Node>> &nodes);
    void setDiagnosticEngine(DiagnosticEngine *engine) { diagnostics = engine; }
    bool hasErrors() const { return hadErrors; }
    const std::unordered_set<std::string> &getGlobals() const { return globals; }
    const std::unordered_map<std::string, std::vector<std::string>> &getParamTypes() const { return paramTypes; }
    const std::unordered_map<std::string, std::string> &getGlobalTypes() const { return globalTypes; }
    const std::unordered_map<std::string, std::string> &getFunctionReturnTypes() const { return functionReturnTypes; }

private:
    struct FieldInfo {
        std::string type;
        bool isStatic = false;
        bool isPrivate = false;
        std::string ownerClass;
    };

    struct MethodInfo {
        std::string returnType;
        std::vector<std::string> paramTypes;
        bool isStatic = false;
        bool isPrivate = false;
        std::string ownerClass;
    };

    struct ClassInfo {
        std::string name;
        std::string base;
        std::unordered_map<std::string, FieldInfo> fields;
        std::unordered_map<std::string, FieldInfo> staticFields;
        std::unordered_map<std::string, MethodInfo> methods;
        std::unordered_map<std::string, MethodInfo> staticMethods;
        std::unordered_map<size_t, std::vector<std::string>> constructors;
    };

    std::vector<std::unordered_map<std::string, std::string>> scopes;
    std::unordered_map<std::string, size_t> functions;
    std::unordered_map<std::string, std::vector<std::string>> paramTypes;
    std::unordered_map<std::string, std::string> functionReturnTypes;
    std::unordered_set<std::string> globals;
    std::unordered_map<std::string, std::string> globalTypes;
    std::unordered_map<std::string, ClassInfo> classes;
    int loopDepth = 0;
    int switchDepth = 0;
    int functionDepth = 0;
    std::string currentClass;
    std::string currentBaseClass;

    void pushScope();
    void popScope();
    void declare(const std::string &name, const std::string &type);
    bool isDeclared(const std::string &name) const;
    std::string lookup(const std::string &name) const;
    bool isClassName(const std::string &name) const;
    const ClassInfo *lookupClass(const std::string &name) const;
    const MethodInfo *lookupMethod(const std::string &className, const std::string &methodName) const;
    const MethodInfo *lookupStaticMethod(const std::string &className, const std::string &methodName) const;
    const FieldInfo *lookupField(const std::string &className, const std::string &fieldName) const;
    const FieldInfo *lookupStaticField(const std::string &className, const std::string &fieldName) const;
    bool isSubclassOf(const std::string &className, const std::string &baseName) const;
    bool isTypeAssignable(const std::string &actualType, const std::string &expectedType) const;
    void collectClassInfo(const std::vector<std::unique_ptr<Node>> &nodes);

    std::string currentType;
    bool lastInputCall = false;
    bool hadErrors = false;
    size_t currentLine = 0;
    size_t currentColumn = 0;
    DiagnosticEngine *diagnostics = nullptr;

    void markNode(const Node &node);
    void reportError(const std::string &message, const std::string &code = "AYM3001");

    void visit(NumberExpr &) override;
    void visit(BoolExpr &) override;
    void visit(StringExpr &) override;
    void visit(VariableExpr &) override;
    void visit(BinaryExpr &) override;
    void visit(UnaryExpr &) override;
    void visit(TernaryExpr &) override;
    void visit(IncDecExpr &) override;
    void visit(CallExpr &) override;
    void visit(MemberCallExpr &) override;
    void visit(NewExpr &) override;
    void visit(FunctionRefExpr &) override;
    void visit(SuperExpr &) override;
    void visit(ListExpr &) override;
    void visit(MapExpr &) override;
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
    void visit(ClassStmt &) override;
    void visit(WhileStmt &) override;
    void visit(DoWhileStmt &) override;
    void visit(SwitchStmt &) override;
    void visit(ImportStmt &) override;
    void visit(ThrowStmt &) override;
    void visit(TryStmt &) override;
}; 

} // namespace aym

#endif // AYM_SEMANTIC_H
