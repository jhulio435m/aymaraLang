#include "codegen.h"
#include "../ast/ast.h"
#include "../builtins/builtins.h"
#include <fstream>
#include <iostream>
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
    };

    std::vector<FunctionInfo> functions;
    std::vector<const Stmt*> mainStmts;
    std::unordered_map<std::string, std::vector<std::string>> paramTypes;
    std::unordered_map<std::string,bool> currentParamStrings;
    std::unordered_map<std::string,bool> currentLocalStrings;
    std::unordered_map<std::string,std::string> globalTypes;
    std::vector<std::string> breakLabels;
    std::vector<std::string> continueLabels;

    void emit(const std::vector<std::unique_ptr<Node>> &nodes,
              const std::string &path,
              const std::unordered_set<std::string> &semGlobals,
              const std::unordered_map<std::string,std::vector<std::string>> &paramTypesIn,
              const std::unordered_map<std::string,std::string> &globalTypesIn,
              bool windows);
private:
    void collectStrings(const Expr *expr);
    void collectLocals(const Stmt *stmt,
                       std::vector<std::string> &locs,
                       std::unordered_map<std::string,bool> &strs);
    void collectGlobal(const Stmt *stmt);

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
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        for (const auto &a : c->getArgs()) collectStrings(a.get());
        return;
    }
}

void CodeGenImpl::collectLocals(const Stmt *stmt,
                                std::vector<std::string> &locs,
                                std::unordered_map<std::string,bool> &strs) {
    if (auto *v = dynamic_cast<const VarDeclStmt*>(stmt)) {
        locs.push_back(v->getName());
        if (v->getType() == "qillqa") strs[v->getName()] = true;
        collectStrings(v->getInit());
        return;
    }
    if (auto *p = dynamic_cast<const PrintStmt*>(stmt)) {
        collectStrings(p->getExpr());
        return;
    }
    if (auto *a = dynamic_cast<const AssignStmt*>(stmt)) {
        collectStrings(a->getValue());
        return;
    }
    if (auto *b = dynamic_cast<const BlockStmt*>(stmt)) {
        for (const auto &s : b->statements) collectLocals(s.get(), locs, strs);
        return;
    }
    if (auto *i = dynamic_cast<const IfStmt*>(stmt)) {
        collectStrings(i->getCondition());
        collectLocals(i->getThen(), locs, strs);
        if (i->getElse()) collectLocals(i->getElse(), locs, strs);
        return;
    }
    if (auto *w = dynamic_cast<const WhileStmt*>(stmt)) {
        collectStrings(w->getCondition());
        collectLocals(w->getBody(), locs, strs);
        return;
    }
    if (auto *dw = dynamic_cast<const DoWhileStmt*>(stmt)) {
        collectLocals(dw->getBody(), locs, strs);
        collectStrings(dw->getCondition());
        return;
    }
    if (auto *f = dynamic_cast<const ForStmt*>(stmt)) {
        collectLocals(f->getInit(), locs, strs);
        collectStrings(f->getCondition());
        collectLocals(f->getPost(), locs, strs);
        collectLocals(f->getBody(), locs, strs);
        return;
    }
    if (auto *sw = dynamic_cast<const SwitchStmt*>(stmt)) {
        collectStrings(sw->getExpr());
        for (const auto &c : sw->getCases()) {
            collectStrings(c.first.get());
            collectLocals(c.second.get(), locs, strs);
        }
        if (sw->getDefault()) collectLocals(sw->getDefault(), locs, strs);
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
    if (auto *p = dynamic_cast<const PrintStmt*>(stmt)) {
        collectStrings(p->getExpr());
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
    currentLocalStrings = info.stringLocals;
    auto pit = paramTypes.find(info.node->getName());
    if (pit != paramTypes.end()) {
        size_t idx = 0;
        for (const auto &p : info.node->getParams()) {
            if (idx < pit->second.size() && pit->second[idx] == "qillqa")
                currentParamStrings[p] = true;
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
            out << "    mov [rbp-" << offsets[p] << "], " << regs[idx] << "\n";
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
        if (auto *s = dynamic_cast<StringExpr *>(p->getExpr())) {
            size_t idx = findString(s->getValue());
            out << "    lea " << reg1(this->windows) << ", [rel fmt_str]\n";
            out << "    lea " << reg2(this->windows) << ", [rel str" << idx << "]\n";
            out << "    xor eax,eax\n";
            out << "    call printf\n";
        } else if (auto *v = dynamic_cast<VariableExpr *>(p->getExpr())) {
            emitExpr(v, locals);
            bool isStr = false;
            if (currentParamStrings.count(v->getName())) {
                isStr = true;
            } else if (!locals && globalTypes.count(v->getName()) && globalTypes[v->getName()] == "qillqa") {
                isStr = true;
            }
            if (isStr) {
                out << "    mov " << reg2(this->windows) << ", rax\n";
                out << "    lea " << reg1(this->windows) << ", [rel fmt_str]\n";
            } else {
                out << "    mov " << reg2(this->windows) << ", rax\n";
                out << "    lea " << reg1(this->windows) << ", [rel fmt_int]\n";
            }
            out << "    xor eax,eax\n";
            out << "    call printf\n";
        } else {
            emitExpr(p->getExpr(), locals);
            out << "    mov " << reg2(this->windows) << ", rax\n";
            out << "    lea " << reg1(this->windows) << ", [rel fmt_int]\n";
            out << "    xor eax,eax\n";
            out << "    call printf\n";
        }
        return;
    }
    if (auto *e = dynamic_cast<const ExprStmt *>(stmt)) {
        emitExpr(e->getExpr(), locals);
        return;
    }
    if (auto *a = dynamic_cast<const AssignStmt *>(stmt)) {
        bool str = false;
        if (locals && currentLocalStrings.count(a->getName())) str = currentLocalStrings[a->getName()];
        else if (!locals && globalTypes.count(a->getName()) && globalTypes[a->getName()] == "qillqa") str = true;

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
    if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt)) {
        if (v->getInit()) {
            bool str = (v->getType() == "qillqa");
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
        emitExpr(w->getCondition(), locals);
        out << "    cmp rax,0\n";
        out << "    je " << end << "\n";
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
        emitExpr(f->getCondition(), locals);
        out << "    cmp rax,0\n";
        out << "    je " << end << "\n";
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
    if (auto *n = dynamic_cast<const NumberExpr *>(expr)) {
        out << "    mov rax, " << n->getValue() << "\n";
        return;
    }
    if (auto *s = dynamic_cast<const StringExpr *>(expr)) {
        size_t idx = findString(s->getValue());
        out << "    lea rax, [rel str" << idx << "]\n";
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
    if (auto *b = dynamic_cast<const BinaryExpr *>(expr)) {
        emitExpr(b->getLeft(), locals);
        out << "    push rax\n";
        emitExpr(b->getRight(), locals);
        out << "    mov rbx, rax\n";
        out << "    pop rax\n";
        switch (b->getOp()) {
            case '+': out << "    add rax, rbx\n"; break;
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
            case '&':
                out << "    cmp rax,0\n    setne al\n    movzx rax,al\n";
                out << "    cmp rbx,0\n    setne bl\n    movzx rbx,bl\n";
                out << "    and rax, rbx\n";
                break;
            case '|':
                out << "    cmp rax,0\n    setne al\n    movzx rax,al\n";
                out << "    cmp rbx,0\n    setne bl\n    movzx rbx,bl\n";
                out << "    or rax, rbx\n";
                break;
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
                out << "    cmp rax, rbx\n    sete al\n    movzx rax,al\n";
                break;
            case 'd':
                out << "    cmp rax, rbx\n    setne al\n    movzx rax,al\n";
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
    if (auto *c = dynamic_cast<const CallExpr *>(expr)) {
        if (c->getName() == BUILTIN_PRINT && !c->getArgs().empty()) {
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
        } else if (c->getName() == BUILTIN_INPUT) {
            out << "    lea " << reg1(this->windows) << ", [rel fmt_read_int]\n";
            out << "    lea " << reg2(this->windows) << ", [rel input_val]\n";
            out << "    xor eax,eax\n";
            out << "    call scanf\n";
            out << "    mov rax, [rel input_val]\n";
            return;
        } else if (c->getName() == BUILTIN_LENGTH) {
            emitExpr(c->getArgs()[0].get(), locals);
            out << "    mov " << reg1(this->windows) << ", rax\n";
            out << "    call strlen\n";
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
                       const std::unordered_map<std::string,std::string> &globalTypesIn,
                       bool windows) {
    this->windows = windows;
    globals = semGlobals;
    paramTypes = paramTypesIn;
    globalTypes = globalTypesIn;

    for (const auto &n : nodes) {
        if (auto *fn = dynamic_cast<FunctionStmt*>(n.get())) {
            FunctionInfo info; info.node = fn;
            info.locals.insert(info.locals.end(), fn->getParams().begin(), fn->getParams().end());
            for (const auto &p : fn->getParams()) {
                info.stringLocals[p] = false;
            }
            collectLocals(fn->getBody(), info.locals, info.stringLocals);
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
    out << "section .data\n";
    out << "fmt_int: db \"%ld\",10,0\n";
    out << "fmt_str: db \"%s\",10,0\n";
    out << "fmt_read_int: db \"%ld\",0\n";
    out << "fmt_read_str: db \"%255s\",0\n";
    out << "input_val: dq 0\n";
    out << "input_buf: times 256 db 0\n";

    for (size_t i=0;i<strings.size();++i) {
        out << "str" << i << ": db \"" << strings[i] << "\",0\n";
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
    if (this->windows) {
        // Reserve shadow space for Win64 calls
        out << "    sub rsp, 32\n";
    } else {
        // Align stack to 16 bytes before calls on SysV ABI
        out << "    sub rsp, 8\n";
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

    fs::create_directories("build");
    fs::create_directories("bin");

    fs::path obj = fs::path(path).replace_extension(windows ? ".obj" : ".o");
    fs::path base = fs::path(path).stem();
    fs::path bin = fs::path("bin") / base;
    if (windows) bin.replace_extension(".exe");
    std::string cmd1 = std::string("nasm ") + (windows ? "-f win64 " : "-felf64 ") + path + " -o " + obj.string();
    if (std::system(cmd1.c_str()) != 0) {
        std::cerr << "Error ensamblando " << path << std::endl;
        return;
    }
    std::string cmd2;
    if (windows)
        cmd2 = "gcc " + obj.string() + " -o " + bin.string();
    else
        cmd2 = "gcc -no-pie " + obj.string() + " -o " + bin.string() + " -lc";
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
                             const std::unordered_map<std::string,std::string> &globalTypes,
                             bool windows) {
    CodeGenImpl impl;
    impl.emit(nodes, outputPath, globals, paramTypes, globalTypes, windows);
}

} // namespace aym

