#include "codegen_impl.h"

namespace aym {

bool CodeGenImpl::emitStmtException(const Stmt *stmt,
                                   const std::unordered_map<std::string,int> *locals,
                                   const std::string &endLabel) {
    if (auto *thr = dynamic_cast<const ThrowStmt *>(stmt)) {
        if (thr->getMessage()) {
            emitExpr(thr->getMessage(), locals);
        } else {
            out << "    mov rax, 0\n";
        }
        int spillPad = this->windows ? 40 : 8;
        out << "    push rax\n";
        out << "    sub rsp, " << spillPad << "\n";
        if (thr->getType()) {
            emitExpr(thr->getType(), locals);
        } else {
            size_t idx = findString("Error");
            out << "    lea rax, [rel str" << idx << "]\n";
        }
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    add rsp, " << spillPad << "\n";
        out << "    pop " << reg2(this->windows) << "\n";
        out << "    call aym_exception_new\n";
        if (!throwFinallyLimitStack.empty() && throwFinallyLimitStack.back() != SIZE_MAX) {
            spillPad = this->windows ? 40 : 8;
            out << "    push rax\n";
            out << "    sub rsp, " << spillPad << "\n";
            size_t limit = throwFinallyLimitStack.back();
            for (size_t i = finallyStack.size(); i > limit; --i) {
                out << "    call " << finallyStack[i - 1] << "\n";
            }
            out << "    add rsp, " << spillPad << "\n";
            out << "    pop rax\n";
        }
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_throw\n";
        return true;
    }
    if (auto *t = dynamic_cast<const TryStmt *>(stmt)) {
        std::string catchLabel = genLabel("catch");
        std::string end = genLabel("tryend");
        std::string finallyLabel;
        if (t->getFinallyBlock()) finallyLabel = genLabel("finally");

        out << "    call aym_try_push\n";
        if (locals && locals->count(t->getHandlerSlot())) {
            out << "    mov [rbp-" << locals->at(t->getHandlerSlot()) << "], rax\n";
        } else {
            out << "    mov [rel " << t->getHandlerSlot() << "], rax\n";
        }
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_try_env\n";
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call setjmp\n";
        out << "    cmp rax,0\n";
        out << "    jne " << catchLabel << "\n";

        if (t->getFinallyBlock()) finallyStack.push_back(finallyLabel);
        emitStmt(t->getTryBlock(), locals, endLabel);
        if (t->getFinallyBlock()) finallyStack.pop_back();

        if (locals && locals->count(t->getHandlerSlot())) {
            out << "    mov " << reg1(this->windows) << ", [rbp-" << locals->at(t->getHandlerSlot()) << "]\n";
        } else {
            out << "    mov " << reg1(this->windows) << ", [rel " << t->getHandlerSlot() << "]\n";
        }
        out << "    call aym_try_pop\n";
        if (t->getFinallyBlock()) out << "    call " << finallyLabel << "\n";
        out << "    jmp " << end << "\n";

        out << catchLabel << ":\n";
        if (locals && locals->count(t->getHandlerSlot())) {
            out << "    mov " << reg1(this->windows) << ", [rbp-" << locals->at(t->getHandlerSlot()) << "]\n";
        } else {
            out << "    mov " << reg1(this->windows) << ", [rel " << t->getHandlerSlot() << "]\n";
        }
        out << "    call aym_try_get_exception\n";
        if (locals && locals->count(t->getExceptionSlot())) {
            out << "    mov [rbp-" << locals->at(t->getExceptionSlot()) << "], rax\n";
        } else {
            out << "    mov [rel " << t->getExceptionSlot() << "], rax\n";
        }
        if (locals && locals->count(t->getHandlerSlot())) {
            out << "    mov " << reg1(this->windows) << ", [rbp-" << locals->at(t->getHandlerSlot()) << "]\n";
        } else {
            out << "    mov " << reg1(this->windows) << ", [rel " << t->getHandlerSlot() << "]\n";
        }
        out << "    call aym_try_pop\n";

        if (t->getCatches().empty()) {
            if (t->getFinallyBlock()) out << "    call " << finallyLabel << "\n";
            if (locals && locals->count(t->getExceptionSlot())) {
                out << "    mov " << reg1(this->windows) << ", [rbp-" << locals->at(t->getExceptionSlot()) << "]\n";
            } else {
                out << "    mov " << reg1(this->windows) << ", [rel " << t->getExceptionSlot() << "]\n";
            }
            out << "    call aym_throw\n";
        } else {
            std::string noMatch = genLabel("catch_nomatch");
            for (size_t idx = 0; idx < t->getCatches().size(); ++idx) {
                const auto &c = t->getCatches()[idx];
                std::string nextLabel = genLabel("catch_next");
                if (!c.typeName.empty()) {
                    if (locals && locals->count(t->getExceptionSlot())) {
                        out << "    mov " << reg1(this->windows) << ", [rbp-" << locals->at(t->getExceptionSlot()) << "]\n";
                    } else {
                        out << "    mov " << reg1(this->windows) << ", [rel " << t->getExceptionSlot() << "]\n";
                    }
                    out << "    call aym_exception_type\n";
                    out << "    mov " << reg2(this->windows) << ", rax\n";
                    size_t typeIdx = findString(c.typeName);
                    out << "    lea " << reg1(this->windows) << ", [rel str" << typeIdx << "]\n";
                    out << "    call strcmp\n";
                    out << "    cmp rax,0\n";
                    out << "    jne " << nextLabel << "\n";
                }
                if (locals && locals->count(c.varName)) {
                    if (locals && locals->count(t->getExceptionSlot())) {
                        out << "    mov rax, [rbp-" << locals->at(t->getExceptionSlot()) << "]\n";
                    } else {
                        out << "    mov rax, [rel " << t->getExceptionSlot() << "]\n";
                    }
                    out << "    mov [rbp-" << locals->at(c.varName) << "], rax\n";
                } else {
                    if (locals && locals->count(t->getExceptionSlot())) {
                        out << "    mov rax, [rbp-" << locals->at(t->getExceptionSlot()) << "]\n";
                    } else {
                        out << "    mov rax, [rel " << t->getExceptionSlot() << "]\n";
                    }
                    out << "    mov [rel " << c.varName << "], rax\n";
                }
                if (t->getFinallyBlock()) finallyStack.push_back(finallyLabel);
                if (t->getFinallyBlock()) {
                    throwFinallyLimitStack.push_back(finallyStack.size() - 1);
                } else {
                    throwFinallyLimitStack.push_back(SIZE_MAX);
                }
                emitStmt(c.block.get(), locals, endLabel);
                throwFinallyLimitStack.pop_back();
                if (t->getFinallyBlock()) finallyStack.pop_back();
                if (t->getFinallyBlock()) out << "    call " << finallyLabel << "\n";
                out << "    jmp " << end << "\n";
                out << nextLabel << ":\n";
            }
            out << noMatch << ":\n";
            if (t->getFinallyBlock()) out << "    call " << finallyLabel << "\n";
            if (locals && locals->count(t->getExceptionSlot())) {
                out << "    mov " << reg1(this->windows) << ", [rbp-" << locals->at(t->getExceptionSlot()) << "]\n";
            } else {
                out << "    mov " << reg1(this->windows) << ", [rel " << t->getExceptionSlot() << "]\n";
            }
            out << "    call aym_throw\n";
        }

        if (t->getFinallyBlock()) {
            out << "    jmp " << end << "\n";
            out << finallyLabel << ":\n";
            int finallySpillPad = this->windows ? 40 : 8;
            out << "    sub rsp, " << finallySpillPad << "\n";
            emitStmt(t->getFinallyBlock(), locals, endLabel);
            out << "    add rsp, " << finallySpillPad << "\n";
            out << "    ret\n";
        }
        out << end << ":\n";
        return true;
    }
    return false;
}

} // namespace aym

