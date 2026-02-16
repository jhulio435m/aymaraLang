#include "codegen_impl.h"
#include "../builtins/builtins.h"

namespace aym {

bool CodeGenImpl::emitBuiltinStringCall(const CallExpr *c,
                                        const std::unordered_map<std::string,int> *locals,
                                        const std::string &nameLower) {
    if (nameLower == BUILTIN_LENGTH || nameLower == BUILTIN_SUYU) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call strlen\n";
        return true;
    }

    if (nameLower == BUILTIN_CHUSA) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_str_trim\n";
        return true;
    }

    if (nameLower == BUILTIN_JALJTA) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_str_split\n";
        return true;
    }

    if (nameLower == BUILTIN_MAYACHTA) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_str_join\n";
        return true;
    }

    if (nameLower == BUILTIN_SIKTA) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_str_replace\n";
        return true;
    }

    if (nameLower == BUILTIN_UTJI) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_str_contains\n";
        return true;
    }

    return false;
}

} // namespace aym
