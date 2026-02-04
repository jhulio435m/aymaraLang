#include "codegen.h"
#include "../ast/ast.h"
#include "../builtins/builtins.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include "../utils/fs.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace aym {

namespace {
size_t labelCounter = 0;
std::string genLabel(const std::string &base) {
    return base + std::to_string(labelCounter++);
}

std::string lowerName(const std::string &value) {
    std::string out = value;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return out;
}

std::string toAsmBytes(const std::string &value) {
    std::ostringstream oss;
    for (size_t i = 0; i < value.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << static_cast<unsigned int>(static_cast<unsigned char>(value[i]));
    }
    if (!value.empty()) oss << ", ";
    oss << "0";
    return oss.str();
}

std::vector<std::string> paramRegs(bool windows) {
    if (windows)
        return {"rcx","rdx","r8","r9"};
    return {"rdi","rsi","rdx","rcx","r8","r9"};
}

std::string reg1(bool windows) { return windows ? "rcx" : "rdi"; }
std::string reg2(bool windows) { return windows ? "rdx" : "rsi"; }
}

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
        const FunctionStmt *node;
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
    std::vector<std::string> breakLabels;
    std::vector<std::string> continueLabels;
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
    bool isStringExpr(const Expr *expr,
                      const std::unordered_map<std::string,int> *locals) const;
    bool isBoolExpr(const Expr *expr,
                    const std::unordered_map<std::string,int> *locals) const;
    bool isListExpr(const Expr *expr,
                    const std::unordered_map<std::string,int> *locals) const;
    std::string listElementType(const Expr *expr,
                                const std::unordered_map<std::string,int> *locals) const;
    void emitPrintValue(const Expr *expr,
                        const std::unordered_map<std::string,int> *locals);
    void emitPrintList(const Expr *expr,
                       const std::unordered_map<std::string,int> *locals);
    void emitPrintDefault(const std::string &label);

    void emitStmt(const Stmt *stmt,
                  const std::unordered_map<std::string,int> *locals,
                  const std::string &endLabel);
    void emitExpr(const Expr *expr,
                  const std::unordered_map<std::string,int> *locals);
    void emitFunction(const FunctionInfo &info);
    void emitInput(bool asString);
};

void CodeGenImpl::collectStrings(const Expr *expr) {
    if (!expr) return;
    if (auto *s = dynamic_cast<const StringExpr*>(expr)) {
        if (std::find(strings.begin(), strings.end(), s->getValue()) == strings.end())
            strings.push_back(s->getValue());
        return;
    }
    if (auto *b = dynamic_cast<const BinaryExpr*>(expr)) {
        collectStrings(b->getLeft());
        collectStrings(b->getRight());
        return;
    }
    if (auto *u = dynamic_cast<const UnaryExpr*>(expr)) {
        collectStrings(u->getExpr());
        return;
    }
    if (auto *t = dynamic_cast<const TernaryExpr*>(expr)) {
        collectStrings(t->getCondition());
        collectStrings(t->getThen());
        collectStrings(t->getElse());
        return;
    }
    if (auto *l = dynamic_cast<const ListExpr*>(expr)) {
        for (const auto &elem : l->getElements()) {
            collectStrings(elem.get());
        }
        return;
    }
    if (auto *i = dynamic_cast<const IndexExpr*>(expr)) {
        collectStrings(i->getBase());
        collectStrings(i->getIndex());
        return;
    }
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        for (const auto &a : c->getArgs()) collectStrings(a.get());
        return;
    }
}

void CodeGenImpl::collectLocals(const Stmt *stmt,
                                std::vector<std::string> &locs,
                                std::unordered_map<std::string,bool> &strs,
                                std::unordered_map<std::string,std::string> &types) {
    if (auto *v = dynamic_cast<const VarDeclStmt*>(stmt)) {
        locs.push_back(v->getName());
        if (v->getType() == "aru") strs[v->getName()] = true;
        std::string declaredType = v->getType();
        if (declaredType == "t'aqa") {
            if (auto *list = dynamic_cast<const ListExpr*>(v->getInit())) {
                bool allStrings = !list->getElements().empty();
                for (const auto &elem : list->getElements()) {
                    if (!dynamic_cast<const StringExpr*>(elem.get())) {
                        allStrings = false;
                        break;
                    }
                }
                declaredType = allStrings ? "t'aqa:aru" : "t'aqa:jakhüwi";
            } else {
                declaredType = "t'aqa:jakhüwi";
            }
        }
        types[v->getName()] = declaredType;
        collectStrings(v->getInit());
        return;
    }
    if (auto *p = dynamic_cast<const PrintStmt*>(stmt)) {
        for (const auto &expr : p->getExprs()) {
            collectStrings(expr.get());
        }
        collectStrings(p->getSeparator());
        collectStrings(p->getTerminator());
        return;
    }
    if (auto *a = dynamic_cast<const AssignStmt*>(stmt)) {
        collectStrings(a->getValue());
        return;
    }
    if (auto *a = dynamic_cast<const IndexAssignStmt*>(stmt)) {
        collectStrings(a->getBase());
        collectStrings(a->getIndex());
        collectStrings(a->getValue());
        return;
    }
    if (auto *b = dynamic_cast<const BlockStmt*>(stmt)) {
        for (const auto &s : b->statements) collectLocals(s.get(), locs, strs, types);
        return;
    }
    if (auto *i = dynamic_cast<const IfStmt*>(stmt)) {
        collectStrings(i->getCondition());
        collectLocals(i->getThen(), locs, strs, types);
        if (i->getElse()) collectLocals(i->getElse(), locs, strs, types);
        return;
    }
    if (auto *w = dynamic_cast<const WhileStmt*>(stmt)) {
        collectStrings(w->getCondition());
        collectLocals(w->getBody(), locs, strs, types);
        return;
    }
    if (auto *dw = dynamic_cast<const DoWhileStmt*>(stmt)) {
        collectLocals(dw->getBody(), locs, strs, types);
        collectStrings(dw->getCondition());
        return;
    }
    if (auto *f = dynamic_cast<const ForStmt*>(stmt)) {
        collectLocals(f->getInit(), locs, strs, types);
        collectStrings(f->getCondition());
        collectLocals(f->getPost(), locs, strs, types);
        collectLocals(f->getBody(), locs, strs, types);
        return;
    }
    if (auto *sw = dynamic_cast<const SwitchStmt*>(stmt)) {
        collectStrings(sw->getExpr());
        for (const auto &c : sw->getCases()) {
            collectStrings(c.first.get());
            collectLocals(c.second.get(), locs, strs, types);
        }
        if (sw->getDefault()) collectLocals(sw->getDefault(), locs, strs, types);
        return;
    }
    if (auto *ret = dynamic_cast<const ReturnStmt*>(stmt)) {
        collectStrings(ret->getValue());
        return;
    }
    if (auto *e = dynamic_cast<const ExprStmt*>(stmt)) {
        collectStrings(e->getExpr());
        return;
    }
}

void CodeGenImpl::collectGlobal(const Stmt *stmt) {
    if (auto *v = dynamic_cast<const VarDeclStmt*>(stmt)) {
        globals.insert(v->getName());
        collectStrings(v->getInit());
        return;
    }
    if (auto *a = dynamic_cast<const AssignStmt*>(stmt)) {
        globals.insert(a->getName());
        collectStrings(a->getValue());
        return;
    }
    if (auto *a = dynamic_cast<const IndexAssignStmt*>(stmt)) {
        collectStrings(a->getBase());
        collectStrings(a->getIndex());
        collectStrings(a->getValue());
        return;
    }
    if (auto *p = dynamic_cast<const PrintStmt*>(stmt)) {
        for (const auto &expr : p->getExprs()) {
            collectStrings(expr.get());
        }
        collectStrings(p->getSeparator());
        collectStrings(p->getTerminator());
        return;
    }
    if (auto *b = dynamic_cast<const BlockStmt*>(stmt)) {
        for (const auto &s : b->statements) collectGlobal(s.get());
        return;
    }
    if (auto *i = dynamic_cast<const IfStmt*>(stmt)) {
        collectStrings(i->getCondition());
        collectGlobal(i->getThen());
        if (i->getElse()) collectGlobal(i->getElse());
        return;
    }
    if (auto *w = dynamic_cast<const WhileStmt*>(stmt)) {
        collectStrings(w->getCondition());
        collectGlobal(w->getBody());
        return;
    }
    if (auto *dw = dynamic_cast<const DoWhileStmt*>(stmt)) {
        collectGlobal(dw->getBody());
        collectStrings(dw->getCondition());
        return;
    }
    if (auto *f = dynamic_cast<const ForStmt*>(stmt)) {
        collectGlobal(f->getInit());
        collectStrings(f->getCondition());
        collectGlobal(f->getPost());
        collectGlobal(f->getBody());
        return;
    }
    if (auto *sw = dynamic_cast<const SwitchStmt*>(stmt)) {
        collectStrings(sw->getExpr());
        for (const auto &c : sw->getCases()) {
            collectStrings(c.first.get());
            collectGlobal(c.second.get());
        }
        if (sw->getDefault()) collectGlobal(sw->getDefault());
        return;
    }
    if (auto *ret = dynamic_cast<const ReturnStmt*>(stmt)) {
        collectStrings(ret->getValue());
        return;
    }
    if (auto *e = dynamic_cast<const ExprStmt*>(stmt)) {
        collectStrings(e->getExpr());
        return;
    }
}

bool CodeGenImpl::isStringExpr(const Expr *expr,
                               const std::unordered_map<std::string,int> *locals) const {
    if (!expr) return false;
    if (dynamic_cast<const StringExpr*>(expr)) return true;
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        std::string nameLower = lowerName(c->getName());
        if (nameLower == BUILTIN_TO_STRING || nameLower == BUILTIN_KATU) return true;
        auto it = functionReturnTypes.find(c->getName());
        if (it == functionReturnTypes.end()) it = functionReturnTypes.find(nameLower);
        if (it != functionReturnTypes.end() && it->second == "aru") return true;
    }
    if (auto *i = dynamic_cast<const IndexExpr*>(expr)) {
        return listElementType(i->getBase(), locals) == "aru";
    }
    if (auto *v = dynamic_cast<const VariableExpr*>(expr)) {
        if (currentParamStrings.count(v->getName()) && currentParamStrings.at(v->getName())) return true;
        if (locals && currentLocalStrings.count(v->getName()) && currentLocalStrings.at(v->getName())) return true;
        if (globalTypes.count(v->getName()) && globalTypes.at(v->getName()) == "aru") return true;
        return false;
    }
    if (auto *b = dynamic_cast<const BinaryExpr*>(expr)) {
        if (b->getOp() == '+') {
            return isStringExpr(b->getLeft(), locals) && isStringExpr(b->getRight(), locals);
        }
        return false;
    }
    if (auto *t = dynamic_cast<const TernaryExpr*>(expr)) {
        return isStringExpr(t->getThen(), locals) && isStringExpr(t->getElse(), locals);
    }
    return false;
}

bool CodeGenImpl::isListExpr(const Expr *expr,
                             const std::unordered_map<std::string,int> *locals) const {
    if (!expr) return false;
    if (dynamic_cast<const ListExpr*>(expr)) return true;
    if (auto *v = dynamic_cast<const VariableExpr*>(expr)) {
        auto it = globalTypes.find(v->getName());
        if (it != globalTypes.end() && it->second.rfind("t'aqa", 0) == 0) return true;
        if (currentParamTypes.count(v->getName()) && currentParamTypes.at(v->getName()).rfind("t'aqa", 0) == 0)
            return true;
        if (locals && currentLocalTypes.count(v->getName()) &&
            currentLocalTypes.at(v->getName()).rfind("t'aqa", 0) == 0)
            return true;
    }
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        std::string name = lowerName(c->getName());
        if (name == BUILTIN_PUSH) return true;
        auto it = functionReturnTypes.find(c->getName());
        if (it == functionReturnTypes.end()) it = functionReturnTypes.find(name);
        if (it != functionReturnTypes.end() && it->second.rfind("t'aqa", 0) == 0) return true;
    }
    return false;
}

std::string CodeGenImpl::listElementType(const Expr *expr,
                                         const std::unordered_map<std::string,int> *locals) const {
    if (!expr) return "";
    if (auto *list = dynamic_cast<const ListExpr*>(expr)) {
        bool seenString = false;
        bool seenNumber = false;
        for (const auto &elem : list->getElements()) {
            if (isStringExpr(elem.get(), locals)) {
                seenString = true;
            } else {
                seenNumber = true;
            }
        }
        if (seenString && !seenNumber) return "aru";
        return "jakhüwi";
    }
    if (auto *v = dynamic_cast<const VariableExpr*>(expr)) {
        auto checkType = [&](const std::unordered_map<std::string,std::string> &types) -> std::string {
            auto it = types.find(v->getName());
            if (it != types.end()) {
                if (it->second.rfind("t'aqa:", 0) == 0) {
                    return it->second.substr(6);
                }
                if (it->second == "t'aqa") {
                    return "jakhüwi";
                }
            }
            return "";
        };
        std::string t;
        if (!currentParamTypes.empty()) {
            t = checkType(currentParamTypes);
            if (!t.empty()) return t;
        }
        if (locals && !currentLocalTypes.empty()) {
            t = checkType(currentLocalTypes);
            if (!t.empty()) return t;
        }
        t = checkType(globalTypes);
        if (!t.empty()) return t;
    }
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        std::string name = lowerName(c->getName());
        if (name == BUILTIN_PUSH && !c->getArgs().empty()) {
            return listElementType(c->getArgs()[0].get(), locals);
        }
    }
    return "";
}

bool CodeGenImpl::isBoolExpr(const Expr *expr,
                             const std::unordered_map<std::string,int> *locals) const {
    if (!expr) return false;
    if (dynamic_cast<const BoolExpr*>(expr)) return true;
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        std::string nameLower = lowerName(c->getName());
        auto it = functionReturnTypes.find(c->getName());
        if (it == functionReturnTypes.end()) it = functionReturnTypes.find(nameLower);
        if (it != functionReturnTypes.end() && it->second == "chiqa") return true;
    }
    if (auto *v = dynamic_cast<const VariableExpr*>(expr)) {
        if (currentParamTypes.count(v->getName()) && currentParamTypes.at(v->getName()) == "chiqa")
            return true;
        if (locals && currentLocalTypes.count(v->getName()) &&
            currentLocalTypes.at(v->getName()) == "chiqa")
            return true;
        if (globalTypes.count(v->getName()) && globalTypes.at(v->getName()) == "chiqa")
            return true;
        return false;
    }
    if (auto *b = dynamic_cast<const BinaryExpr*>(expr)) {
        char op = b->getOp();
        if (op == '&' || op == '|' || op == 's' || op == 'd' || op == '<' || op == '>' ||
            op == 'l' || op == 'g') {
            return true;
        }
        return false;
    }
    if (auto *u = dynamic_cast<const UnaryExpr*>(expr)) {
        return u->getOp() == '!';
    }
    if (auto *t = dynamic_cast<const TernaryExpr*>(expr)) {
        return isBoolExpr(t->getThen(), locals) && isBoolExpr(t->getElse(), locals);
    }
    return false;
}

void CodeGenImpl::emitPrintDefault(const std::string &label) {
    out << "    lea " << reg1(this->windows) << ", [rel fmt_raw]\n";
    out << "    lea " << reg2(this->windows) << ", [rel " << label << "]\n";
    out << "    xor eax,eax\n";
    out << "    call printf\n";
}

void CodeGenImpl::emitPrintValue(const Expr *expr,
                                 const std::unordered_map<std::string,int> *locals) {
    if (!expr) return;
    if (isListExpr(expr, locals)) {
        emitPrintList(expr, locals);
        return;
    }
    if (isBoolExpr(expr, locals)) {
        std::string falseLbl = genLabel("bool_false");
        std::string endLbl = genLabel("bool_end");
        emitExpr(expr, locals);
        out << "    cmp rax,0\n";
        out << "    je " << falseLbl << "\n";
        emitPrintDefault("bool_true");
        out << "    jmp " << endLbl << "\n";
        out << falseLbl << ":\n";
        emitPrintDefault("bool_false");
        out << endLbl << ":\n";
        return;
    }
    emitExpr(expr, locals);
    if (isStringExpr(expr, locals)) {
        out << "    mov " << reg2(this->windows) << ", rax\n";
        out << "    lea " << reg1(this->windows) << ", [rel fmt_raw]\n";
    } else {
        out << "    mov " << reg2(this->windows) << ", rax\n";
        out << "    lea " << reg1(this->windows) << ", [rel fmt_int_raw]\n";
    }
    out << "    xor eax,eax\n";
    out << "    call printf\n";
}

void CodeGenImpl::emitPrintList(const Expr *expr,
                                const std::unordered_map<std::string,int> *locals) {
    std::string loop = genLabel("list_loop");
    std::string end = genLabel("list_end");
    std::string elemType = listElementType(expr, locals);
    emitExpr(expr, locals);
    out << "    mov rbx, rax\n";
    emitPrintDefault("list_open");
    out << "    mov " << reg1(this->windows) << ", rbx\n";
    out << "    call aym_array_length\n";
    out << "    mov r12, rax\n";
    out << "    xor r13, r13\n";
    out << loop << ":\n";
    out << "    cmp r13, r12\n";
    out << "    je " << end << "\n";
    out << "    mov " << reg2(this->windows) << ", r13\n";
    out << "    mov " << reg1(this->windows) << ", rbx\n";
    out << "    call aym_array_get\n";
    if (elemType == "aru") {
        out << "    mov " << reg2(this->windows) << ", rax\n";
        out << "    lea " << reg1(this->windows) << ", [rel fmt_raw]\n";
    } else {
        out << "    mov " << reg2(this->windows) << ", rax\n";
        out << "    lea " << reg1(this->windows) << ", [rel fmt_int_raw]\n";
    }
    out << "    xor eax,eax\n";
    out << "    call printf\n";
    out << "    inc r13\n";
    out << "    cmp r13, r12\n";
    out << "    je " << end << "\n";
    emitPrintDefault("list_sep");
    out << "    jmp " << loop << "\n";
    out << end << ":\n";
    emitPrintDefault("list_close");
}

void CodeGenImpl::emitFunction(const FunctionInfo &info) {
    std::unordered_map<std::string,int> offsets;
    int off = 0;
    for (const auto &n : info.locals) {
        off += 8;
        offsets[n] = off;
    }
    // Reserve Win64 shadow space (32 bytes) so calls are ABI-compliant
    int shadow = this->windows ? 32 : 0;
    int stackSize = (off + shadow + 15) & ~15;

    currentParamStrings.clear();
    currentParamTypes.clear();
    currentLocalStrings = info.stringLocals;
    currentLocalTypes = info.localTypes;
    auto pit = paramTypes.find(info.node->getName());
    if (pit != paramTypes.end()) {
        size_t idx = 0;
        for (const auto &p : info.node->getParams()) {
            if (idx < pit->second.size()) {
                currentParamTypes[p.name] = pit->second[idx];
                if (pit->second[idx] == "aru") currentParamStrings[p.name] = true;
            }
            ++idx;
        }
    }

    std::string endLabel = genLabel("endfunc");

    out << info.node->getName() << ":\n";
    out << "    push rbp\n";
    out << "    mov rbp, rsp\n";
    // rbx is callee-saved on both SysV and Win64 ABIs
    out << "    push rbx\n";
    if (stackSize) out << "    sub rsp, " << stackSize << "\n";

    // store parameters
    std::vector<std::string> regs = paramRegs(this->windows);
    size_t idx = 0;
    for (const auto &p : info.node->getParams()) {
        if (idx < regs.size()) {
            out << "    mov [rbp-" << offsets[p.name] << "], " << regs[idx] << "\n";
        }
        ++idx;
    }

    emitStmt(info.node->getBody(), &offsets, endLabel);

    out << endLabel << ":\n";
    if (stackSize) out << "    add rsp, " << stackSize << "\n";
    out << "    pop rbx\n";
    out << "    pop rbp\n";
    out << "    ret\n";
}

void CodeGenImpl::emitStmt(const Stmt *stmt,
                           const std::unordered_map<std::string,int> *locals,
                           const std::string &endLabel) {
    if (auto *p = dynamic_cast<const PrintStmt *>(stmt)) {
        const auto &exprs = p->getExprs();
        const Expr *sepExpr = p->getSeparator();
        const Expr *termExpr = p->getTerminator();
        for (size_t i = 0; i < exprs.size(); ++i) {
            emitPrintValue(exprs[i].get(), locals);
            if (i + 1 < exprs.size()) {
                if (sepExpr) emitPrintValue(sepExpr, locals);
                else emitPrintDefault("print_sep");
            }
        }
        if (termExpr) emitPrintValue(termExpr, locals);
        else emitPrintDefault("print_term");
        return;
    }
    if (auto *e = dynamic_cast<const ExprStmt *>(stmt)) {
        if (e->getExpr()) emitExpr(e->getExpr(), locals);
        return;
    }
    if (auto *a = dynamic_cast<const AssignStmt *>(stmt)) {
        bool str = false;
        if (locals && currentLocalStrings.count(a->getName())) str = currentLocalStrings[a->getName()];
        else if (!locals && globalTypes.count(a->getName()) && globalTypes[a->getName()] == "aru") str = true;

        if (auto *call = dynamic_cast<const CallExpr*>(a->getValue()); call && call->getName()==BUILTIN_INPUT) {
            if (str)
                emitInput(true);
            else
                emitInput(false);
        } else {
            emitExpr(a->getValue(), locals);
        }
        if (locals && locals->count(a->getName())) {
            out << "    mov [rbp-" << locals->at(a->getName()) << "], rax\n";
        } else {
            out << "    mov [rel " << a->getName() << "], rax\n";
        }
        return;
    }
    if (auto *a = dynamic_cast<const IndexAssignStmt *>(stmt)) {
        std::vector<std::string> regs = paramRegs(this->windows);
        emitExpr(a->getValue(), locals);
        out << "    mov " << regs[2] << ", rax\n";
        emitExpr(a->getIndex(), locals);
        out << "    mov " << regs[1] << ", rax\n";
        emitExpr(a->getBase(), locals);
        out << "    mov " << regs[0] << ", rax\n";
        out << "    call aym_array_set\n";
        return;
    }
    if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt)) {
        if (v->getInit()) {
            bool str = (v->getType() == "aru");
            if (auto *call = dynamic_cast<const CallExpr*>(v->getInit()); call && call->getName()==BUILTIN_INPUT) {
                emitInput(str);
            } else {
                emitExpr(v->getInit(), locals);
            }
            if (locals && locals->count(v->getName())) {
                out << "    mov [rbp-" << locals->at(v->getName()) << "], rax\n";
            } else {
                out << "    mov [rel " << v->getName() << "], rax\n";
            }
        }
        return;
    }
    if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
        for (const auto &s : b->statements) emitStmt(s.get(), locals, endLabel);
        return;
    }
    if (auto *i = dynamic_cast<const IfStmt *>(stmt)) {
        std::string elseLbl = genLabel("else");
        std::string end = genLabel("endif");
        emitExpr(i->getCondition(), locals);
        out << "    cmp rax,0\n";
        if (i->getElse()) {
            out << "    je " << elseLbl << "\n";
            emitStmt(i->getThen(), locals, endLabel);
            out << "    jmp " << end << "\n";
            out << elseLbl << ":\n";
            emitStmt(i->getElse(), locals, endLabel);
        } else {
            out << "    je " << end << "\n";
            emitStmt(i->getThen(), locals, endLabel);
        }
        out << end << ":\n";
        return;
    }
    if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) {
        std::string loop = genLabel("loop");
        std::string cont = genLabel("cont");
        std::string end = genLabel("endloop");
        breakLabels.push_back(end);
        continueLabels.push_back(cont);
        out << loop << ":\n";
        if (w->getCondition()) {
            emitExpr(w->getCondition(), locals);
            out << "    cmp rax,0\n";
            out << "    je " << end << "\n";
        }
        emitStmt(w->getBody(), locals, endLabel);
        out << cont << ":\n";
        out << "    jmp " << loop << "\n";
        out << end << ":\n";
        breakLabels.pop_back();
        continueLabels.pop_back();
        return;
    }
    if (auto *f = dynamic_cast<const ForStmt *>(stmt)) {
        std::string loop = genLabel("forloop");
        std::string cont = genLabel("forcont");
        std::string end = genLabel("forend");
        emitStmt(f->getInit(), locals, endLabel);
        breakLabels.push_back(end);
        continueLabels.push_back(cont);
        out << loop << ":\n";
        if (f->getCondition()) {
            emitExpr(f->getCondition(), locals);
            out << "    cmp rax,0\n";
            out << "    je " << end << "\n";
        }
        emitStmt(f->getBody(), locals, endLabel);
        out << cont << ":\n";
        emitStmt(f->getPost(), locals, endLabel);
        out << "    jmp " << loop << "\n";
        out << end << ":\n";
        breakLabels.pop_back();
        continueLabels.pop_back();
        return;
    }
    if (auto *dw = dynamic_cast<const DoWhileStmt *>(stmt)) {
        std::string loop = genLabel("doloop");
        std::string cont = genLabel("docont");
        std::string end = genLabel("doend");
        breakLabels.push_back(end);
        continueLabels.push_back(cont);
        out << loop << ":\n";
        emitStmt(dw->getBody(), locals, endLabel);
        out << cont << ":\n";
        emitExpr(dw->getCondition(), locals);
        out << "    cmp rax,0\n";
        out << "    jne " << loop << "\n";
        out << end << ":\n";
        breakLabels.pop_back();
        continueLabels.pop_back();
        return;
    }
    if (auto *sw = dynamic_cast<const SwitchStmt *>(stmt)) {
        emitExpr(sw->getExpr(), locals);
        out << "    mov rbx, rax\n";
        std::string end = genLabel("switchend");
        breakLabels.push_back(end);
        std::vector<std::string> labels;
        for (size_t i = 0; i < sw->getCases().size(); ++i)
            labels.push_back(genLabel("case"));
        std::string defLabel = sw->getDefault() ? genLabel("defcase") : end;
        size_t idx = 0;
        for (const auto &c : sw->getCases()) {
            emitExpr(c.first.get(), locals);
            out << "    cmp rbx, rax\n";
            out << "    je " << labels[idx] << "\n";
            ++idx;
        }
        if (sw->getDefault())
            out << "    jmp " << defLabel << "\n";
        else
            out << "    jmp " << end << "\n";
        idx = 0;
        for (const auto &c : sw->getCases()) {
            out << labels[idx] << ":\n";
            emitStmt(c.second.get(), locals, endLabel);
            ++idx;
        }
        if (sw->getDefault()) {
            out << defLabel << ":\n";
            emitStmt(sw->getDefault(), locals, endLabel);
        }
        out << end << ":\n";
        breakLabels.pop_back();
        return;
    }
    if (dynamic_cast<const BreakStmt *>(stmt)) {
        if (!breakLabels.empty())
            out << "    jmp " << breakLabels.back() << "\n";
        return;
    }
    if (dynamic_cast<const ContinueStmt *>(stmt)) {
        if (!continueLabels.empty())
            out << "    jmp " << continueLabels.back() << "\n";
        return;
    }
    if (auto *ret = dynamic_cast<const ReturnStmt *>(stmt)) {
        if (ret->getValue()) emitExpr(ret->getValue(), locals);
        out << "    jmp " << endLabel << "\n";
        return;
    }
}

void CodeGenImpl::emitExpr(const Expr *expr,
                           const std::unordered_map<std::string,int> *locals) {
    if (!expr) return;
    if (auto *n = dynamic_cast<const NumberExpr *>(expr)) {
        out << "    mov rax, " << n->getValue() << "\n";
        return;
    }
    if (auto *b = dynamic_cast<const BoolExpr *>(expr)) {
        out << "    mov rax, " << (b->getValue() ? 1 : 0) << "\n";
        return;
    }
    if (auto *s = dynamic_cast<const StringExpr *>(expr)) {
        size_t idx = findString(s->getValue());
        out << "    lea rax, [rel str" << idx << "]\n";
        return;
    }
    if (auto *l = dynamic_cast<const ListExpr *>(expr)) {
        out << "    mov " << reg1(this->windows) << ", " << l->getElements().size() << "\n";
        out << "    call aym_array_new\n";
        out << "    mov rbx, rax\n";
        size_t idx = 0;
        for (const auto &elem : l->getElements()) {
            emitExpr(elem.get(), locals);
            std::vector<std::string> regs = paramRegs(this->windows);
            out << "    mov " << regs[2] << ", rax\n";
            out << "    mov " << regs[1] << ", " << idx << "\n";
            out << "    mov " << regs[0] << ", rbx\n";
            out << "    call aym_array_set\n";
            ++idx;
        }
        out << "    mov rax, rbx\n";
        return;
    }
    if (auto *i = dynamic_cast<const IndexExpr *>(expr)) {
        emitExpr(i->getIndex(), locals);
        out << "    mov " << reg2(this->windows) << ", rax\n";
        emitExpr(i->getBase(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_array_get\n";
        return;
    }
    if (auto *v = dynamic_cast<const VariableExpr *>(expr)) {
        if (locals && locals->count(v->getName())) {
            out << "    mov rax, [rbp-" << locals->at(v->getName()) << "]\n";
        } else {
            out << "    mov rax, [rel " << v->getName() << "]\n";
        }
        return;
    }
    if (auto *inc = dynamic_cast<const IncDecExpr *>(expr)) {
        bool isLocal = locals && locals->count(inc->getName());
        if (isLocal) {
            out << "    mov rax, [rbp-" << locals->at(inc->getName()) << "]\n";
        } else {
            out << "    mov rax, [rel " << inc->getName() << "]\n";
        }
        if (inc->prefix()) {
            if (inc->increment()) out << "    add rax, 1\n";
            else out << "    sub rax, 1\n";
            if (isLocal) {
                out << "    mov [rbp-" << locals->at(inc->getName()) << "], rax\n";
            } else {
                out << "    mov [rel " << inc->getName() << "], rax\n";
            }
        } else {
            out << "    mov rbx, rax\n";
            if (inc->increment()) out << "    add rax, 1\n";
            else out << "    sub rax, 1\n";
            if (isLocal) {
                out << "    mov [rbp-" << locals->at(inc->getName()) << "], rax\n";
            } else {
                out << "    mov [rel " << inc->getName() << "], rax\n";
            }
            out << "    mov rax, rbx\n";
        }
        return;
    }
    if (auto *b = dynamic_cast<const BinaryExpr *>(expr)) {
        if (b->getOp() == '&') {
            std::string falseLbl = genLabel("and_false");
            std::string endLbl = genLabel("and_end");
            emitExpr(b->getLeft(), locals);
            out << "    cmp rax,0\n";
            out << "    je " << falseLbl << "\n";
            emitExpr(b->getRight(), locals);
            out << "    cmp rax,0\n";
            out << "    setne al\n";
            out << "    movzx rax,al\n";
            out << "    jmp " << endLbl << "\n";
            out << falseLbl << ":\n";
            out << "    mov rax,0\n";
            out << endLbl << ":\n";
            return;
        }
        if (b->getOp() == '|') {
            std::string trueLbl = genLabel("or_true");
            std::string endLbl = genLabel("or_end");
            emitExpr(b->getLeft(), locals);
            out << "    cmp rax,0\n";
            out << "    jne " << trueLbl << "\n";
            emitExpr(b->getRight(), locals);
            out << "    cmp rax,0\n";
            out << "    setne al\n";
            out << "    movzx rax,al\n";
            out << "    jmp " << endLbl << "\n";
            out << trueLbl << ":\n";
            out << "    mov rax,1\n";
            out << endLbl << ":\n";
            return;
        }
        emitExpr(b->getLeft(), locals);
        out << "    push rax\n";
        out << "    sub rsp, 8\n";
        emitExpr(b->getRight(), locals);
        out << "    add rsp, 8\n";
        out << "    mov rbx, rax\n";
        out << "    pop rax\n";
        bool leftIsString = isStringExpr(b->getLeft(), locals);
        bool rightIsString = isStringExpr(b->getRight(), locals);
        switch (b->getOp()) {
            case '+':
                if (leftIsString && rightIsString) {
                    out << "    mov " << reg1(this->windows) << ", rax\n";
                    out << "    mov " << reg2(this->windows) << ", rbx\n";
                    out << "    call aym_str_concat\n";
                } else {
                    out << "    add rax, rbx\n";
                }
                break;
            case '-': out << "    sub rax, rbx\n"; break;
            case '*': out << "    imul rax, rbx\n"; break;
            case '/': out << "    cqo\n    idiv rbx\n"; break;
            case '%': out << "    cqo\n    idiv rbx\n    mov rax, rdx\n"; break;
            case '^': {
                std::string loop = genLabel("pow");
                std::string end = genLabel("powend");
                out << "    mov rcx, rbx\n";
                out << "    mov rbx, rax\n";
                out << "    mov rax,1\n";
                out << loop << ":\n";
                out << "    cmp rcx,0\n";
                out << "    je " << end << "\n";
                out << "    imul rax, rbx\n";
                out << "    dec rcx\n";
                out << "    jmp " << loop << "\n";
                out << end << ":\n";
                break;
            }
            case '<':
                out << "    cmp rax, rbx\n    setl al\n    movzx rax,al\n";
                break;
            case 'l':
                out << "    cmp rax, rbx\n    setle al\n    movzx rax,al\n";
                break;
            case '>':
                out << "    cmp rax, rbx\n    setg al\n    movzx rax,al\n";
                break;
            case 'g':
                out << "    cmp rax, rbx\n    setge al\n    movzx rax,al\n";
                break;
            case 's':
                if (leftIsString && rightIsString) {
                    out << "    mov " << reg1(this->windows) << ", rax\n";
                    out << "    mov " << reg2(this->windows) << ", rbx\n";
                    out << "    call strcmp\n";
                    out << "    cmp rax,0\n    sete al\n    movzx rax,al\n";
                } else {
                    out << "    cmp rax, rbx\n    sete al\n    movzx rax,al\n";
                }
                break;
            case 'd':
                if (leftIsString && rightIsString) {
                    out << "    mov " << reg1(this->windows) << ", rax\n";
                    out << "    mov " << reg2(this->windows) << ", rbx\n";
                    out << "    call strcmp\n";
                    out << "    cmp rax,0\n    setne al\n    movzx rax,al\n";
                } else {
                    out << "    cmp rax, rbx\n    setne al\n    movzx rax,al\n";
                }
                break;
        }
        return;
    }
    if (auto *u = dynamic_cast<const UnaryExpr *>(expr)) {
        emitExpr(u->getExpr(), locals);
        switch (u->getOp()) {
            case '!':
                out << "    cmp rax,0\n";
                out << "    sete al\n";
                out << "    movzx rax,al\n";
                break;
            case '-':
                out << "    neg rax\n";
                break;
            default:
                break; // '+' is a no-op
        }
        return;
    }
    if (auto *t = dynamic_cast<const TernaryExpr *>(expr)) {
        std::string elseLbl = genLabel("tern_else");
        std::string endLbl = genLabel("tern_end");
        emitExpr(t->getCondition(), locals);
        out << "    cmp rax,0\n";
        out << "    je " << elseLbl << "\n";
        emitExpr(t->getThen(), locals);
        out << "    jmp " << endLbl << "\n";
        out << elseLbl << ":\n";
        emitExpr(t->getElse(), locals);
        out << endLbl << ":\n";
        return;
    }
    if (auto *c = dynamic_cast<const CallExpr *>(expr)) {
        std::string nameLower = lowerName(c->getName());
        if (nameLower == BUILTIN_PRINT && !c->getArgs().empty()) {
            if (auto *s = dynamic_cast<StringExpr *>(c->getArgs()[0].get())) {
                size_t idx = findString(s->getValue());
                out << "    lea " << reg1(this->windows) << ", [rel fmt_str]\n";
                out << "    lea " << reg2(this->windows) << ", [rel str" << idx << "]\n";
                out << "    xor eax,eax\n";
                out << "    call printf\n";
            } else {
                emitExpr(c->getArgs()[0].get(), locals);
                out << "    mov " << reg2(this->windows) << ", rax\n";
                out << "    lea " << reg1(this->windows) << ", [rel fmt_int]\n";
                out << "    xor eax,eax\n";
                out << "    call printf\n";
            }
            return;
        } else if (nameLower == BUILTIN_INPUT) {
            out << "    lea " << reg1(this->windows) << ", [rel fmt_read_int]\n";
            out << "    lea " << reg2(this->windows) << ", [rel input_val]\n";
            out << "    xor eax,eax\n";
            out << "    call scanf\n";
            out << "    mov rax, [rel input_val]\n";
            return;
        } else if (nameLower == BUILTIN_KATU) {
            if (!c->getArgs().empty()) {
                emitExpr(c->getArgs()[0].get(), locals);
                out << "    mov " << reg2(this->windows) << ", rax\n";
                out << "    lea " << reg1(this->windows) << ", [rel fmt_raw]\n";
                out << "    xor eax,eax\n";
                out << "    call printf\n";
            }
            if (c->getArgs().size() > 1) {
                emitExpr(c->getArgs()[1].get(), locals);
                out << "    mov " << reg2(this->windows) << ", rax\n";
                out << "    lea " << reg1(this->windows) << ", [rel fmt_raw]\n";
                out << "    xor eax,eax\n";
                out << "    call printf\n";
            }
            out << "    lea " << reg1(this->windows) << ", [rel fmt_read_str]\n";
            out << "    lea " << reg2(this->windows) << ", [rel input_buf]\n";
            out << "    xor eax,eax\n";
            out << "    call scanf\n";
            out << "    lea rax, [rel input_buf]\n";
            return;
        } else if (nameLower == BUILTIN_TO_STRING) {
            if (c->getArgs().empty()) return;
            const Expr *arg = c->getArgs()[0].get();
            if (isStringExpr(arg, locals)) {
                emitExpr(arg, locals);
                return;
            }
            if (isBoolExpr(arg, locals)) {
                std::string falseLbl = genLabel("bool_false");
                std::string endLbl = genLabel("bool_end");
                emitExpr(arg, locals);
                out << "    cmp rax,0\n";
                out << "    je " << falseLbl << "\n";
                out << "    lea rax, [rel bool_true]\n";
                out << "    jmp " << endLbl << "\n";
                out << falseLbl << ":\n";
                out << "    lea rax, [rel bool_false]\n";
                out << endLbl << ":\n";
                return;
            }
            emitExpr(arg, locals);
            out << "    mov " << reg1(this->windows) << ", rax\n";
            out << "    call aym_to_string\n";
            return;
        } else if (nameLower == BUILTIN_TO_NUMBER) {
            if (c->getArgs().empty()) return;
            const Expr *arg = c->getArgs()[0].get();
            if (isStringExpr(arg, locals)) {
                emitExpr(arg, locals);
                out << "    mov " << reg1(this->windows) << ", rax\n";
                out << "    call aym_to_number\n";
                return;
            }
            emitExpr(arg, locals);
            return;
        } else if (nameLower == BUILTIN_LARGO) {
            emitExpr(c->getArgs()[0].get(), locals);
            out << "    mov " << reg1(this->windows) << ", rax\n";
            out << "    call aym_array_length\n";
            return;
        } else if (nameLower == BUILTIN_PUSH) {
            std::vector<std::string> regs = paramRegs(this->windows);
            emitExpr(c->getArgs()[1].get(), locals);
            out << "    mov " << regs[1] << ", rax\n";
            emitExpr(c->getArgs()[0].get(), locals);
            out << "    mov " << regs[0] << ", rax\n";
            out << "    call aym_array_push\n";
            return;
        } else if (nameLower == BUILTIN_LENGTH) {
            emitExpr(c->getArgs()[0].get(), locals);
            out << "    mov " << reg1(this->windows) << ", rax\n";
            out << "    call strlen\n";
            return;
        } else if (nameLower == BUILTIN_RANDOM) {
            emitExpr(c->getArgs()[0].get(), locals);
            out << "    mov " << reg1(this->windows) << ", rax\n";
            out << "    call aym_random\n";
            return;
        } else if (nameLower == BUILTIN_SLEEP) {
            emitExpr(c->getArgs()[0].get(), locals);
            out << "    mov " << reg1(this->windows) << ", rax\n";
            out << "    call aym_sleep\n";
            return;
        } else if (nameLower == BUILTIN_ARRAY_NEW) {
            emitExpr(c->getArgs()[0].get(), locals);
            out << "    mov " << reg1(this->windows) << ", rax\n";
            out << "    call aym_array_new\n";
            return;
        } else if (nameLower == BUILTIN_ARRAY_GET) {
            emitExpr(c->getArgs()[1].get(), locals);
            out << "    mov " << reg2(this->windows) << ", rax\n";
            emitExpr(c->getArgs()[0].get(), locals);
            out << "    mov " << reg1(this->windows) << ", rax\n";
            out << "    call aym_array_get\n";
            return;
        } else if (nameLower == BUILTIN_ARRAY_SET) {
            std::vector<std::string> regs = paramRegs(this->windows);
            emitExpr(c->getArgs()[2].get(), locals);
            out << "    mov " << regs[2] << ", rax\n";
            emitExpr(c->getArgs()[1].get(), locals);
            out << "    mov " << regs[1] << ", rax\n";
            emitExpr(c->getArgs()[0].get(), locals);
            out << "    mov " << regs[0] << ", rax\n";
            out << "    call aym_array_set\n";
            return;
        } else if (nameLower == BUILTIN_ARRAY_FREE) {
            emitExpr(c->getArgs()[0].get(), locals);
            out << "    mov " << reg1(this->windows) << ", rax\n";
            out << "    call aym_array_free\n";
            return;
        } else if (nameLower == BUILTIN_ARRAY_LENGTH) {
            emitExpr(c->getArgs()[0].get(), locals);
            out << "    mov " << reg1(this->windows) << ", rax\n";
            out << "    call aym_array_length\n";
            return;
        } else if (nameLower == BUILTIN_WRITE) {
            emitExpr(c->getArgs()[0].get(), locals);
            out << "    mov " << reg2(this->windows) << ", rax\n";
            out << "    lea " << reg1(this->windows) << ", [rel fmt_raw]\n";
            out << "    xor eax,eax\n";
            out << "    call printf\n";
            return;
        }
        // user function call
        std::vector<std::string> regs = paramRegs(this->windows);
        size_t idx = 0;
        for (const auto &a : c->getArgs()) {
            emitExpr(a.get(), locals);
            if (idx < regs.size()) {
                out << "    mov " << regs[idx] << ", rax\n";
            }
            ++idx;
        }
        out << "    call " << c->getName() << "\n";
        return;
    }
}

void CodeGenImpl::emitInput(bool asString) {
    if (asString) {
        out << "    lea " << reg1(this->windows) << ", [rel fmt_read_str]\n";
        out << "    lea " << reg2(this->windows) << ", [rel input_buf]\n";
        out << "    xor eax,eax\n";
        out << "    call scanf\n";
        out << "    lea rax, [rel input_buf]\n";
    } else {
        out << "    lea " << reg1(this->windows) << ", [rel fmt_read_int]\n";
        out << "    lea " << reg2(this->windows) << ", [rel input_val]\n";
        out << "    xor eax,eax\n";
        out << "    call scanf\n";
        out << "    mov rax, [rel input_val]\n";
    }
}

void CodeGenImpl::emit(const std::vector<std::unique_ptr<Node>> &nodes,
                       const std::string &path,
                       const std::unordered_set<std::string> &semGlobals,
                       const std::unordered_map<std::string,std::vector<std::string>> &paramTypesIn,
                       const std::unordered_map<std::string,std::string> &functionReturnTypesIn,
                       const std::unordered_map<std::string,std::string> &globalTypesIn,
                       bool windows,
                       long seedIn,
                       const std::string &runtimeDirIn) {
    this->windows = windows;
    globals = semGlobals;
    paramTypes = paramTypesIn;
    functionReturnTypes = functionReturnTypesIn;
    globalTypes = globalTypesIn;
    seed = seedIn;

    for (const auto &n : nodes) {
        if (auto *fn = dynamic_cast<FunctionStmt*>(n.get())) {
            FunctionInfo info; info.node = fn;
            for (const auto &param : fn->getParams()) {
                info.locals.push_back(param.name);
            }
            for (const auto &p : fn->getParams()) {
                info.stringLocals[p.name] = false;
            }
            collectLocals(fn->getBody(), info.locals, info.stringLocals, info.localTypes);
            functions.push_back(std::move(info));
        } else {
            mainStmts.push_back(static_cast<const Stmt*>(n.get()));
            collectGlobal(static_cast<const Stmt*>(n.get()));
        }
    }

    std::ofstream fout(path);
    out.swap(fout);

    out << "extern printf\n";
    out << "extern scanf\n";
    out << "extern strlen\n";
    out << "extern strcmp\n";
    out << "extern aym_random\n";
    out << "extern aym_srand\n";
    out << "extern aym_sleep\n";
    out << "extern aym_array_new\n";
    out << "extern aym_array_get\n";
    out << "extern aym_array_set\n";
    out << "extern aym_array_free\n";
    out << "extern aym_array_length\n";
    out << "extern aym_array_push\n";
    out << "extern aym_str_concat\n";
    out << "extern aym_to_string\n";
    out << "extern aym_to_number\n";
    out << "section .data\n";
    out << "fmt_int: db \"%ld\",10,0\n";
    out << "fmt_str: db \"%s\",10,0\n";
    out << "fmt_raw: db \"%s\",0\n";
    out << "fmt_int_raw: db \"%ld\",0\n";
    out << "fmt_read_int: db \"%ld\",0\n";
    out << "fmt_read_str: db \"%255s\",0\n";
    out << "input_val: dq 0\n";
    out << "input_buf: times 256 db 0\n";
    out << "print_sep: db \" \",0\n";
    out << "print_term: db 10,0\n";
    out << "list_open: db \"[\",0\n";
    out << "list_sep: db \", \",0\n";
    out << "list_close: db \"]\",0\n";
    out << "bool_true: db \"utji\",0\n";
    out << "bool_false: db \"janiutji\",0\n";

    for (size_t i = 0; i < strings.size(); ++i) {
        out << "str" << i << ": db " << toAsmBytes(strings[i]) << "\n";
    }
    for (const auto &g : globals) {
        out << g << ": dq 0\n";
    }

    out << "section .text\n";
    out << "global main\n";

    for (const auto &f : functions) emitFunction(f);

    out << "main:\n";
    out << "    push rbp\n";
    out << "    mov rbp, rsp\n";
    // Preserve rbx (callee-saved) as we use it in expressions
    out << "    push rbx\n";
    currentParamStrings.clear();
    currentLocalStrings.clear();
    currentParamTypes.clear();
    currentLocalTypes.clear();
    if (this->windows) {
        // Reserve shadow space for Win64 calls
        out << "    sub rsp, 32\n";
    } else {
        // Align stack to 16 bytes before calls on SysV ABI
        out << "    sub rsp, 8\n";
    }
    if (seed >= 0) {
        out << "    mov " << reg1(this->windows) << ", " << seed << "\n";
        out << "    call aym_srand\n";
    }
    std::string mainEnd = genLabel("endmain");
    for (const auto *s : mainStmts) emitStmt(s, nullptr, mainEnd);
    out << mainEnd << ":\n";
    if (this->windows) {
        out << "    add rsp, 32\n";
    } else {
        out << "    add rsp, 8\n";
    }
    out << "    pop rbx\n";
    out << "    pop rbp\n";
    out << "    mov eax,0\n";
    out << "    ret\n";

    if (!windows)
        out << "section .note.GNU-stack noalloc noexec nowrite progbits\n";

    out.close();

    fs::path asmPath = fs::path(path);
    fs::path outputDir = asmPath.has_parent_path() ? asmPath.parent_path() : fs::current_path();
    if (!outputDir.empty()) {
        fs::create_directories(outputDir);
    }

    fs::path obj = asmPath;
    obj.replace_extension(windows ? ".obj" : ".o");
    fs::path base = asmPath.stem();
    fs::path bin = outputDir / base;
    if (windows) bin.replace_extension(".exe");
    auto quoted = [](const fs::path &p) {
        std::string value = p.string();
        if (value.find(' ') != std::string::npos) {
            return std::string("\"") + value + "\"";
        }
        return value;
    };
    std::string cmd1 = std::string("nasm ") + (windows ? "-f win64 " : "-felf64 ") + quoted(asmPath) + " -o " + quoted(obj);
    if (std::system(cmd1.c_str()) != 0) {
        std::cerr << "Error ensamblando " << path << std::endl;
        return;
    }
    std::string cmd2;
    fs::path runtimeDir = runtimeDirIn.empty() ? fs::path("runtime") : fs::path(runtimeDirIn);
    fs::path runtimeC = runtimeDir / "runtime.c";
    fs::path mathC = runtimeDir / "math.c";
    if (windows)
        cmd2 = "gcc " + quoted(obj) + " " + quoted(runtimeC) + " " + quoted(mathC) + " -o " + quoted(bin) + " -lm";
    else
        cmd2 = "gcc -no-pie " + quoted(obj) + " " + quoted(runtimeC) + " " + quoted(mathC) + " -o " + quoted(bin) + " -lm -lc";
    if (std::system(cmd2.c_str()) != 0) {
        std::cerr << "Error enlazando " << obj.string() << std::endl;
        return;
    }
    std::cout << "[aymc] Ejecutable generado: " << bin.string() << std::endl;
}


void CodeGenerator::generate(const std::vector<std::unique_ptr<Node>> &nodes,
                             const std::string &outputPath,
                             const std::unordered_set<std::string> &globals,
                             const std::unordered_map<std::string,std::vector<std::string>> &paramTypes,
                             const std::unordered_map<std::string,std::string> &functionReturnTypes,
                             const std::unordered_map<std::string,std::string> &globalTypes,
                             bool windows,
                             long seed,
                             const std::string &runtimeDir) {
    CodeGenImpl impl;
    impl.emit(nodes, outputPath, globals, paramTypes, functionReturnTypes, globalTypes, windows, seed, runtimeDir);
}

} // namespace aym
