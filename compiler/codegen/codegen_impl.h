#ifndef AYM_CODEGEN_IMPL_H
#define AYM_CODEGEN_IMPL_H

#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "codegen_helpers.h"
#include "codegen.h"
#include "../ast/ast.h"

namespace aym {

class CodeGenImpl {
public:
    std::ofstream out;
    std::unordered_set<std::string> globals;
    std::vector<std::string> strings;
    bool windows = false;
    size_t findString(const std::string &val) const {
        for (size_t i = 0; i < strings.size(); ++i) {
            if (strings[i] == val) return i;
        }
        return strings.size();
    }

    struct FunctionInfo {
        std::string name;
        std::vector<Param> params;
        const BlockStmt *body;
        std::vector<std::string> locals;
        std::unordered_map<std::string,bool> stringLocals;
        std::unordered_map<std::string,std::string> localTypes;
    };

    std::vector<FunctionInfo> functions;
    std::vector<const Stmt*> mainStmts;
    std::unordered_map<std::string, std::vector<std::string>> paramTypes;
    std::unordered_map<std::string,bool> currentParamStrings;
    std::unordered_map<std::string,bool> currentLocalStrings;
    std::unordered_map<std::string,std::string> currentParamTypes;
    std::unordered_map<std::string,std::string> currentLocalTypes;
    std::unordered_map<std::string,std::string> globalTypes;
    std::unordered_map<std::string,std::string> functionReturnTypes;
    std::unordered_map<std::string, const ClassStmt*> classes;
    std::string currentClass;
    std::vector<std::string> breakLabels;
    std::vector<std::string> continueLabels;
    std::vector<std::string> finallyStack;
    std::vector<size_t> loopFinallyDepth;
    std::vector<size_t> throwFinallyLimitStack;
    size_t tryTempCounter = 0;
    long seed = -1;

    void emit(const std::vector<std::unique_ptr<Node>> &nodes,
              const std::string &path,
              const std::unordered_set<std::string> &semGlobals,
              const std::unordered_map<std::string,std::vector<std::string>> &paramTypesIn,
              const std::unordered_map<std::string,std::string> &functionReturnTypesIn,
              const std::unordered_map<std::string,std::string> &globalTypesIn,
              bool windows,
              long seedIn,
              const std::string &runtimeDirIn);
private:
    void collectStrings(const Expr *expr);
    void collectLocals(const Stmt *stmt,
                       std::vector<std::string> &locs,
                       std::unordered_map<std::string,bool> &strs,
                       std::unordered_map<std::string,std::string> &types);
    void collectGlobal(const Stmt *stmt);
    void assignTrySlots(Stmt *stmt);
    bool isStringExpr(const Expr *expr,
                      const std::unordered_map<std::string,int> *locals) const;
    bool isBoolExpr(const Expr *expr,
                    const std::unordered_map<std::string,int> *locals) const;
    bool isListExpr(const Expr *expr,
                    const std::unordered_map<std::string,int> *locals) const;
    bool isMapExpr(const Expr *expr,
                   const std::unordered_map<std::string,int> *locals) const;
    std::string listElementType(const Expr *expr,
                                const std::unordered_map<std::string,int> *locals) const;
    std::string mapValueType(const Expr *expr,
                             const std::unordered_map<std::string,int> *locals) const;
    void emitPrintValue(const Expr *expr,
                        const std::unordered_map<std::string,int> *locals);
    void emitPrintList(const Expr *expr,
                       const std::unordered_map<std::string,int> *locals);
    void emitPrintMap(const Expr *expr,
                      const std::unordered_map<std::string,int> *locals);
    void emitPrintDefault(const std::string &label);

    void emitStmt(const Stmt *stmt,
                  const std::unordered_map<std::string,int> *locals,
                  const std::string &endLabel);
    void emitExpr(const Expr *expr,
                  const std::unordered_map<std::string,int> *locals);
    void emitFunction(const FunctionInfo &info);
    void emitInput(bool asString);

    void registerClass(const ClassStmt *cls);
    void collectClassStrings();
    bool emitNewExpr(const NewExpr *expr,
                     const std::unordered_map<std::string,int> *locals);
    bool emitMemberCallExpr(const MemberCallExpr *expr,
                            const std::unordered_map<std::string,int> *locals);
    bool emitSuperExpr(const SuperExpr *expr,
                       const std::unordered_map<std::string,int> *locals);
};

} // namespace aym

#endif // AYM_CODEGEN_IMPL_H
