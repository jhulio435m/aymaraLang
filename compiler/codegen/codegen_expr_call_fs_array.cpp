#include "codegen_impl.h"
#include "../builtins/builtins.h"

namespace aym {

bool CodeGenImpl::emitBuiltinFsCall(const CallExpr *c,
                                    const std::unordered_map<std::string,int> *locals,
                                    const std::string &nameLower) {
    if (nameLower == BUILTIN_ULLANA_ARU) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_fs_read_text\n";
        return true;
    }

    if (nameLower == BUILTIN_QILLQANA_ARU) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_fs_write_text\n";
        return true;
    }

    if (nameLower == BUILTIN_UTJI_ARKATA) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_fs_exists\n";
        return true;
    }

    return false;
}

bool CodeGenImpl::emitBuiltinArrayPrimitiveCall(const CallExpr *c,
                                                const std::unordered_map<std::string,int> *locals,
                                                const std::string &nameLower) {
    if (nameLower == BUILTIN_ARRAY_NEW) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_array_new\n";
        return true;
    }

    if (nameLower == BUILTIN_ARRAY_GET) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_array_get\n";
        return true;
    }

    if (nameLower == BUILTIN_ARRAY_SET) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_array_set\n";
        return true;
    }

    if (nameLower == BUILTIN_ARRAY_FREE) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_array_free\n";
        return true;
    }

    if (nameLower == BUILTIN_ARRAY_LENGTH) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_array_length\n";
        return true;
    }

    return false;
}

} // namespace aym
