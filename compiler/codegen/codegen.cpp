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
    std::vector<const FunctionStmt*> functions;
    std::vector<std::pair<std::string, std::string>> loopStack; // break, continue

    void emit(const std::vector<std::unique_ptr<Node>> &nodes, const std::string &path);
private:
    void emitStmt(const Stmt *stmt);
    void emitExpr(const Expr *expr);
    void collect(const Stmt *stmt);
};

void CodeGenImpl::emit(const std::vector<std::unique_ptr<Node>> &nodes, const std::string &path) {
    out.open(path);
    out << "extern printf\n";
    out << "extern leer_linea\n";
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
    for (const auto *fn : functions) {
        out << fn->getName() << ":\n";
        const auto &ps = fn->getParams();
        if (ps.size() > 0)
            out << "    mov [rel " << ps[0] << "], rdi\n";
        if (ps.size() > 1)
            out << "    mov [rel " << ps[1] << "], rsi\n";
        emitStmt(fn->getBody());
        out << "    ret\n";
    }
    out << "global main\n";
    out << "main:\n";
    for (const auto &n : nodes) {
        if (dynamic_cast<const FunctionStmt*>(n.get())) continue;
        emitStmt(static_cast<Stmt*>(n.get()));
    }
    out << "    mov eax,0\n";
    out << "    ret\n";
    out.close();

    std::string obj = path.substr(0, path.find_last_of('.')) + ".o";
    std::string bin = path.substr(0, path.find_last_of('.'));
    std::string cmd1 = "nasm -felf64 " + path + " -o " + obj;
    std::string cmdRuntime = "gcc -c runtime/runtime.c -o runtime/runtime.o";
    std::string cmd2 = "gcc -no-pie " + obj + " runtime/runtime.o -o " + bin + " -lc";
    if (std::system(cmd1.c_str()) != 0 || std::system(cmdRuntime.c_str()) != 0 || std::system(cmd2.c_str()) != 0) {
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
    if (auto *a = dynamic_cast<const AssignStmt *>(stmt)) {
        emitExpr(a->getValue());
        out << "    mov [rel " << a->getName() << "], rax\n";
        return;
    }
    if (dynamic_cast<const VarDeclStmt *>(stmt)) {
        // declarations handled in collect phase
        if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt)) {
            if (v->getInit()) {
                emitExpr(v->getInit());
                out << "    mov [rel " << v->getName() << "], rax\n";
            }
        }
        return;
    }
    if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
        for (const auto &s : b->statements) emitStmt(s.get());
        return;
    }
    if (auto *i = dynamic_cast<const IfStmt *>(stmt)) {
        std::string elseLbl = genLabel("else");
        std::string end = genLabel("endif");
        emitExpr(i->getCondition());
        out << "    cmp rax,0\n";
        if (i->getElse()) {
            out << "    je " << elseLbl << "\n";
            emitStmt(i->getThen());
            out << "    jmp " << end << "\n";
            out << elseLbl << ":\n";
            emitStmt(i->getElse());
        } else {
            out << "    je " << end << "\n";
            emitStmt(i->getThen());
        }
        out << end << ":\n";
        return;
    }
    if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) {
        std::string loop = genLabel("loop");
        std::string end = genLabel("endloop");
        loopStack.push_back({end, loop});
        out << loop << ":\n";
        emitExpr(w->getCondition());
        out << "    cmp rax,0\n";
        out << "    je " << end << "\n";
        emitStmt(w->getBody());
        out << "    jmp " << loop << "\n";
        out << end << ":\n";
        loopStack.pop_back();
        return;
    }
    if (auto *f = dynamic_cast<const ForStmt *>(stmt)) {
        std::string loop = genLabel("forloop");
        std::string cont = genLabel("forcont");
        std::string end = genLabel("forend");
        emitStmt(f->getInit());
        loopStack.push_back({end, cont});
        out << loop << ":\n";
        emitExpr(f->getCondition());
        out << "    cmp rax,0\n";
        out << "    je " << end << "\n";
        emitStmt(f->getBody());
        out << cont << ":\n";
        emitStmt(f->getPost());
        out << "    jmp " << loop << "\n";
        out << end << ":\n";
        loopStack.pop_back();
        return;
    }
    if (dynamic_cast<const BreakStmt *>(stmt)) {
        if (!loopStack.empty()) out << "    jmp " << loopStack.back().first << "\n";
        return;
    }
    if (dynamic_cast<const ContinueStmt *>(stmt)) {
        if (!loopStack.empty()) out << "    jmp " << loopStack.back().second << "\n";
        return;
    }
    if (auto *ret = dynamic_cast<const ReturnStmt *>(stmt)) {
        if (ret->getValue()) emitExpr(ret->getValue());
        out << "    ret\n";
        return;
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
        } else {
            if (!c->getArgs().empty()) {
                emitExpr(c->getArgs()[0].get());
                out << "    mov rdi, rax\n";
            }
            if (c->getArgs().size() > 1) {
                emitExpr(c->getArgs()[1].get());
                out << "    mov rsi, rax\n";
            }
            out << "    call " << c->getName() << "\n";
        }
        return;
    }
}

void CodeGenImpl::collect(const Stmt *stmt) {
    if (auto *p = dynamic_cast<const PrintStmt *>(stmt)) {
        if (auto *s = dynamic_cast<StringExpr *>(p->getExpr())) {
            strings.push_back(s->getValue());
        }
        return;
    }
    if (auto *a = dynamic_cast<const AssignStmt *>(stmt)) {
        variables.insert(a->getName());
        return;
    }
    if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt)) {
        variables.insert(v->getName());
        return;
    }
    if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
        for (const auto &st : b->statements) collect(st.get());
        return;
    }
    if (auto *i = dynamic_cast<const IfStmt *>(stmt)) {
        collect(i->getThen());
        if (i->getElse()) collect(i->getElse());
        return;
    }
    if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) {
        collect(w->getBody());
        return;
    }
    if (auto *f = dynamic_cast<const ForStmt *>(stmt)) {
        collect(f->getInit());
        collect(f->getBody());
        collect(f->getPost());
        return;
    }
    if (auto *fn = dynamic_cast<const FunctionStmt *>(stmt)) {
        functions.push_back(fn);
        for (const auto &p : fn->getParams()) variables.insert(p);
        collect(fn->getBody());
        return;
    }
}

void CodeGenerator::generate(const std::vector<std::unique_ptr<Node>> &nodes,
                             const std::string &outputPath) {
    CodeGenImpl impl;
    impl.emit(nodes, outputPath);
}

} // namespace aym
