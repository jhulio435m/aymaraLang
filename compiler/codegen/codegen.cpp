#include "codegen.h"
#include "../ast/ast.h"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <unordered_set>

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
    std::unordered_set<std::string> variables;
    std::vector<std::string> strings;

    void emit(const std::vector<std::unique_ptr<Node>> &nodes, const std::string &path);
private:
    void emitStmt(const Stmt *stmt);
    void emitExpr(const Expr *expr);
    void collect(const Stmt *stmt);
    void collectExpr(const Expr *expr);
};

void CodeGenImpl::emit(const std::vector<std::unique_ptr<Node>> &nodes, const std::string &path) {
    out.open(path);
    out << "extern printf\n";
    out << "section .data\n";
    out << "fmt_int: db \"%ld\",10,0\n";
    out << "fmt_str: db \"%s\",10,0\n";

    for (const auto &n : nodes) {
        collect(static_cast<const Stmt *>(n.get()));
    }

    for (size_t i = 0; i < strings.size(); ++i) {
        out << "str" << i << ": db \"" << strings[i] << "\",0\n";
    }
    for (const auto &v : variables) {
        out << v << ": dq 0\n";
    }

    out << "section .text\n";
    out << "global main\n";
    out << "main:\n";
    for (const auto &n : nodes) emitStmt(static_cast<Stmt*>(n.get()));
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

void CodeGenImpl::emitStmt(const Stmt *stmt) {
    if (auto *p = dynamic_cast<const PrintStmt *>(stmt)) {
        if (auto *s = dynamic_cast<StringExpr *>(p->getExpr())) {
            size_t idx = 0;
            for (; idx < strings.size(); ++idx) {
                if (strings[idx] == s->getValue()) break;
            }
            out << "    lea rdi, [rel fmt_str]\n";
            out << "    lea rsi, [rel str" << idx << "]\n";
            out << "    xor eax,eax\n";
            out << "    call printf\n";
        } else {
            emitExpr(p->getExpr());
            out << "    mov rsi, rax\n";
            out << "    lea rdi, [rel fmt_int]\n";
            out << "    xor eax,eax\n";
            out << "    call printf\n";
        }
        return;
    }
    if (auto *e = dynamic_cast<const ExprStmt *>(stmt)) {
        emitExpr(e->getExpr());
        return;
    }
    if (auto *a = dynamic_cast<const AssignStmt *>(stmt)) {
        emitExpr(a->getValue());
        out << "    mov [rel " << a->getName() << "], rax\n";
        return;
    }
    if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
        for (const auto &s : b->statements) emitStmt(s.get());
        return;
    }
    if (auto *i = dynamic_cast<const IfStmt *>(stmt)) {
        std::string end = genLabel("endif");
        emitExpr(i->getCondition());
        out << "    cmp rax,0\n";
        out << "    je " << end << "\n";
        emitStmt(i->getThen());
        out << end << ":\n";
        return;
    }
    if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) {
        if (auto *num = dynamic_cast<NumberExpr *>(w->getCondition())) {
            std::string loop = genLabel("loop");
            std::string end = genLabel("endloop");
            out << "    mov rbx, " << num->getValue() << "\n";
            out << loop << ":\n";
            out << "    cmp rbx,0\n";
            out << "    je " << end << "\n";
            emitStmt(w->getBody());
            out << "    dec rbx\n";
            out << "    jmp " << loop << "\n";
            out << end << ":\n";
            return;
        } else {
            std::string loop = genLabel("loop");
            std::string end = genLabel("endloop");
            out << loop << ":\n";
            emitExpr(w->getCondition());
            out << "    cmp rax,0\n";
            out << "    je " << end << "\n";
            emitStmt(w->getBody());
            out << "    jmp " << loop << "\n";
            out << end << ":\n";
            return;
        }
    }
}

void CodeGenImpl::emitExpr(const Expr *expr) {
    if (auto *n = dynamic_cast<const NumberExpr *>(expr)) {
        out << "    mov rax, " << n->getValue() << "\n";
        return;
    }
    if (auto *s = dynamic_cast<const StringExpr *>(expr)) {
        size_t idx = 0;
        for (; idx < strings.size(); ++idx) {
            if (strings[idx] == s->getValue()) break;
        }
        out << "    lea rax, [rel str" << idx << "]\n";
        return;
    }
    if (auto *v = dynamic_cast<const VariableExpr *>(expr)) {
        out << "    mov rax, [rel " << v->getName() << "]\n";
        return;
    }
    if (auto *b = dynamic_cast<const BinaryExpr *>(expr)) {
        emitExpr(b->getLeft());
        out << "    push rax\n";
        emitExpr(b->getRight());
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
                size_t idx = 0;
                for (; idx < strings.size(); ++idx) if (strings[idx] == s->getValue()) break;
                out << "    lea rdi, [rel fmt_str]\n";
                out << "    lea rsi, [rel str" << idx << "]\n";
                out << "    xor eax,eax\n";
                out << "    call printf\n";
            } else {
                emitExpr(c->getArgs()[0].get());
                out << "    mov rsi, rax\n";
                out << "    lea rdi, [rel fmt_int]\n";
                out << "    xor eax,eax\n";
                out << "    call printf\n";
            }
        }
        return;
    }
}

void CodeGenImpl::collect(const Stmt *stmt) {
    if (auto *p = dynamic_cast<const PrintStmt *>(stmt)) {
        collectExpr(p->getExpr());
        return;
    }
    if (auto *e = dynamic_cast<const ExprStmt *>(stmt)) {
        collectExpr(e->getExpr());
        return;
    }
    if (auto *a = dynamic_cast<const AssignStmt *>(stmt)) {
        variables.insert(a->getName());
        return;
    }
    if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
        for (const auto &st : b->statements) collect(st.get());
        return;
    }
    if (auto *i = dynamic_cast<const IfStmt *>(stmt)) {
        collect(i->getThen());
        return;
    }
    if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) {
        collect(w->getBody());
        return;
    }
}

void CodeGenImpl::collectExpr(const Expr *expr) {
    if (auto *s = dynamic_cast<const StringExpr *>(expr)) {
        strings.push_back(s->getValue());
        return;
    }
    if (auto *b = dynamic_cast<const BinaryExpr *>(expr)) {
        collectExpr(b->getLeft());
        collectExpr(b->getRight());
        return;
    }
    if (auto *c = dynamic_cast<const CallExpr *>(expr)) {
        for (const auto &a : c->getArgs()) collectExpr(a.get());
        return;
    }
}

void CodeGenerator::generate(const std::vector<std::unique_ptr<Node>> &nodes,
                             const std::string &outputPath) {
    CodeGenImpl impl;
    impl.emit(nodes, outputPath);
}

} // namespace aym
