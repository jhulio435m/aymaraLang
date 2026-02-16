#include "codegen_impl.h"
#include "../builtins/builtins.h"

namespace aym {

bool CodeGenImpl::emitBuiltinCollectionCall(const CallExpr *c,
                                            const std::unordered_map<std::string,int> *locals,
                                            const std::string &nameLower) {
    if (nameLower == BUILTIN_LARGO || nameLower == BUILTIN_SUYUT) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_array_length\n";
        return true;
    }

    if (nameLower == BUILTIN_PUSH || nameLower == BUILTIN_CHULLU) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_array_push\n";
        return true;
    }

    if (nameLower == BUILTIN_APSU) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_array_pop\n";
        return true;
    }

    if (nameLower == BUILTIN_APSU_UKA) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_array_remove_at\n";
        return true;
    }

    if (nameLower == BUILTIN_UTJIT) {
        emitCallArgs(c->getArgs(), locals, 0);
        if (listElementType(c->getArgs()[0].get(), locals) == "aru") {
            out << "    call aym_array_contains_str\n";
        } else {
            out << "    call aym_array_contains_int\n";
        }
        return true;
    }

    if (nameLower == BUILTIN_THAQHA) {
        emitCallArgs(c->getArgs(), locals, 0);
        if (listElementType(c->getArgs()[0].get(), locals) == "aru") {
            out << "    call aym_array_find_str\n";
        } else {
            out << "    call aym_array_find_int\n";
        }
        return true;
    }

    if (nameLower == BUILTIN_UTJI_SUTI) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_map_contains\n";
        return true;
    }

    if (nameLower == BUILTIN_SUYU_M) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_map_size\n";
        return true;
    }

    if (nameLower == BUILTIN_SUTINAKA) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_map_keys\n";
        return true;
    }

    if (nameLower == BUILTIN_CHANINAKA) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_map_values\n";
        return true;
    }

    if (nameLower == BUILTIN_APSU_SUTI) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_map_delete\n";
        return true;
    }

    if (nameLower == BUILTIN_CHANI_M) {
        emitCallArgs(c->getArgs(), locals, 0);
        if (c->getArgs().size() == 3) {
            out << "    call aym_map_get_default\n";
        } else {
            out << "    call aym_map_get\n";
        }
        return true;
    }

    if (nameLower == BUILTIN_WAKICHA) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        if (listElementType(c->getArgs()[0].get(), locals) == "aru") {
            out << "    call aym_array_sort_str\n";
        } else {
            out << "    call aym_array_sort_int\n";
        }
        return true;
    }

    if (nameLower == BUILTIN_SAPAKI) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        if (listElementType(c->getArgs()[0].get(), locals) == "aru") {
            out << "    call aym_array_unique_str\n";
        } else {
            out << "    call aym_array_unique_int\n";
        }
        return true;
    }

    return false;
}

} // namespace aym
