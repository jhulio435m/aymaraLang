#include "codegen_impl.h"
#include "../builtins/builtins.h"

namespace aym {

bool CodeGenImpl::emitBuiltinMathCall(const CallExpr *c,
                                      const std::unordered_map<std::string,int> *locals,
                                      const std::string &nameLower) {
    if (nameLower == BUILTIN_SIN) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_sin\n";
        return true;
    }
    if (nameLower == BUILTIN_COS) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_cos\n";
        return true;
    }
    if (nameLower == BUILTIN_TAN) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_tan\n";
        return true;
    }
    if (nameLower == BUILTIN_ASIN) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_asin\n";
        return true;
    }
    if (nameLower == BUILTIN_ACOS) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_acos\n";
        return true;
    }
    if (nameLower == BUILTIN_ATAN) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_atan\n";
        return true;
    }
    if (nameLower == BUILTIN_SQRT) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_sqrt\n";
        return true;
    }
    if (nameLower == BUILTIN_POW) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_pow\n";
        return true;
    }
    if (nameLower == BUILTIN_EXP) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_exp\n";
        return true;
    }
    if (nameLower == BUILTIN_LOG) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_log\n";
        return true;
    }
    if (nameLower == BUILTIN_LOG10) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_log10\n";
        return true;
    }
    if (nameLower == BUILTIN_FLOOR) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_floor\n";
        return true;
    }
    if (nameLower == BUILTIN_CEIL) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_ceil\n";
        return true;
    }
    if (nameLower == BUILTIN_ROUND) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_round\n";
        return true;
    }
    if (nameLower == BUILTIN_FABS) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_fabs\n";
        return true;
    }
    return false;
}

} // namespace aym
