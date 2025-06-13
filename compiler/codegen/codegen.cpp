#include "codegen.h"
#include "../ast/ast.h"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace aym {

namespace {
size_t labelCounter = 0;
std::string genLabel(const std::string &base) {
    return base + std::to_string(labelCounter++);
}
}

class CodeGenImpl {
public:
    std::ofstream out;
    std::unordered_set<std::string> globals;
    std::vector<std::string> strings;
    size_t findString(const std::string &val) const {
        for (size_t i = 0; i < strings.size(); ++i) {
            if (strings[i] == val) return i;
        }
        return strings.size();
    }

    struct FunctionInfo {
        const FunctionStmt *node;
        std::vector<std::string> locals;
    };

    std::vector<FunctionInfo> functions;
    std::vector<const Stmt*> mainStmts;

    void emit(const std::vector<std::unique_ptr<Node>> &nodes,
              const std::string &path,
              const std::unordered_set<std::string> &semGlobals);
private:
    void collectStrings(const Expr *expr);
    void collectLocals(const Stmt *stmt, std::vector<std::string> &locs);
    void collectGlobal(const Stmt *stmt);

    void emitStmt(const Stmt *stmt,
                  const std::unordered_map<std::string,int> *locals,
                  const std::string &endLabel);
    void emitExpr(const Expr *expr,
                  const std::unordered_map<std::string,int> *locals);
    void emitFunction(const FunctionInfo &info);
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
    if (auto *c = dynamic_cast<const CallExpr*>(expr)) {
        for (const auto &a : c->getArgs()) collectStrings(a.get());
        return;
    }
}

void CodeGenImpl::collectLocals(const Stmt *stmt, std::vector<std::string> &locs) {
    if (auto *v = dynamic_cast<const VarDeclStmt*>(stmt)) {
        locs.push_back(v->getName());
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
        for (const auto &s : b->statements) collectLocals(s.get(), locs);
        return;
    }
    if (auto *i = dynamic_cast<const IfStmt*>(stmt)) {
        collectStrings(i->getCondition());
        collectLocals(i->getThen(), locs);
        if (i->getElse()) collectLocals(i->getElse(), locs);
        return;
    }
    if (auto *w = dynamic_cast<const WhileStmt*>(stmt)) {
        collectStrings(w->getCondition());
        collectLocals(w->getBody(), locs);
        return;
    }
    if (auto *f = dynamic_cast<const ForStmt*>(stmt)) {
        collectLocals(f->getInit(), locs);
        collectStrings(f->getCondition());
        collectLocals(f->getPost(), locs);
        collectLocals(f->getBody(), locs);
        return;
    }
    if (auto *ret = dynamic_cast<const ReturnStmt*>(stmt)) {
        collectStrings(ret->getValue());
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
    if (auto *f = dynamic_cast<const ForStmt*>(stmt)) {
        collectGlobal(f->getInit());
        collectStrings(f->getCondition());
        collectGlobal(f->getPost());
        collectGlobal(f->getBody());
        return;
    }
    if (auto *ret = dynamic_cast<const ReturnStmt*>(stmt)) {
        collectStrings(ret->getValue());
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
    int stackSize = (off + 15) & ~15;

    std::string endLabel = genLabel("endfunc");

    out << info.node->getName() << ":\n";
    out << "    push rbp\n";
    out << "    mov rbp, rsp\n";
    out << "    push rbx\n";
    if (stackSize) out << "    sub rsp, " << stackSize << "\n";

    // store parameters
    std::vector<std::string> regs = {"rdi","rsi","rdx","rcx","r8","r9"};
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
            out << "    lea rdi, [rel fmt_str]\n";
            out << "    lea rsi, [rel str" << idx << "]\n";
            out << "    xor eax,eax\n";
            out << "    call printf\n";
        } else {
            emitExpr(p->getExpr(), locals);
            out << "    mov rsi, rax\n";
            out << "    lea rdi, [rel fmt_int]\n";
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
        emitExpr(a->getValue(), locals);
        if (locals && locals->count(a->getName())) {
            out << "    mov [rbp-" << locals->at(a->getName()) << "], rax\n";
        } else {
            out << "    mov [rel " << a->getName() << "], rax\n";
        }
        return;
    }
    if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt)) {
        if (v->getInit()) {
            emitExpr(v->getInit(), locals);
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
        std::string end = genLabel("endif");
        emitExpr(i->getCondition(), locals);
        out << "    cmp rax,0\n";
        out << "    je " << end << "\n";
        emitStmt(i->getThen(), locals, endLabel);
        out << end << ":\n";
        return;
    }
    if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) {
        std::string loop = genLabel("loop");
        std::string end = genLabel("endloop");
        out << loop << ":\n";
        emitExpr(w->getCondition(), locals);
        out << "    cmp rax,0\n";
        out << "    je " << end << "\n";
        emitStmt(w->getBody(), locals, endLabel);
        out << "    jmp " << loop << "\n";
        out << end << ":\n";
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
        }
        return;
    }
    if (auto *c = dynamic_cast<const CallExpr *>(expr)) {
        if (c->getName() == "willt’aña" && !c->getArgs().empty()) {
            if (auto *s = dynamic_cast<StringExpr *>(c->getArgs()[0].get())) {
                size_t idx = findString(s->getValue());
                out << "    lea rdi, [rel fmt_str]\n";
                out << "    lea rsi, [rel str" << idx << "]\n";
                out << "    xor eax,eax\n";
                out << "    call printf\n";
            } else {
                emitExpr(c->getArgs()[0].get(), locals);
                out << "    mov rsi, rax\n";
                out << "    lea rdi, [rel fmt_int]\n";
                out << "    xor eax,eax\n";
                out << "    call printf\n";
            }
            return;
        }
        // user function call
        std::vector<std::string> regs = {"rdi","rsi","rdx","rcx","r8","r9"};
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

void CodeGenImpl::emit(const std::vector<std::unique_ptr<Node>> &nodes,
                       const std::string &path,
                       const std::unordered_set<std::string> &semGlobals) {
    globals = semGlobals;

    for (const auto &n : nodes) {
        if (auto *fn = dynamic_cast<FunctionStmt*>(n.get())) {
            FunctionInfo info; info.node = fn;
            info.locals.insert(info.locals.end(), fn->getParams().begin(), fn->getParams().end());
            collectLocals(fn->getBody(), info.locals);
            functions.push_back(std::move(info));
        } else {
            mainStmts.push_back(static_cast<const Stmt*>(n.get()));
            collectGlobal(static_cast<const Stmt*>(n.get()));
        }
    }

    std::ofstream fout(path);
    out.swap(fout);

    out << "extern printf\n";
    out << "section .data\n";
    out << "fmt_int: db \"%ld\",10,0\n";
    out << "fmt_str: db \"%s\",10,0\n";

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
    std::string mainEnd = genLabel("endmain");
    for (const auto *s : mainStmts) emitStmt(s, nullptr, mainEnd);
    out << mainEnd << ":\n";
    out << "    mov eax,0\n";
    out << "    ret\n";

    out.close();

    std::string obj = path.substr(0, path.find_last_of('.')) + ".o";
    std::string bin = path.substr(0, path.find_last_of('.'));
    std::string cmd1 = "nasm -felf64 " + path + " -o " + obj;
    std::string cmd2 = "gcc -no-pie " + obj + " -o " + bin + " -lc";
    if (std::system(cmd1.c_str()) != 0 || std::system(cmd2.c_str()) != 0) {
        std::cerr << "Error assembling or linking" << std::endl;
    }
}


void CodeGenerator::generate(const std::vector<std::unique_ptr<Node>> &nodes,
                             const std::string &outputPath,
                             const std::unordered_set<std::string> &globals) {
    CodeGenImpl impl;
    impl.emit(nodes, outputPath, globals);
}

} // namespace aym

