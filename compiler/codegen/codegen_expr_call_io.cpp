#include "codegen_impl.h"
#include "../builtins/builtins.h"

namespace aym {

bool CodeGenImpl::emitBuiltinIoCall(const CallExpr *c,
                                    const std::unordered_map<std::string,int> *locals,
                                    const std::string &nameLower) {
    if (nameLower == BUILTIN_PRINT && !c->getArgs().empty()) {
        if (auto *s = dynamic_cast<const StringExpr *>(c->getArgs()[0].get())) {
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
        return true;
    }

    if (nameLower == BUILTIN_INPUT) {
        out << "    lea " << reg1(this->windows) << ", [rel fmt_read_int]\n";
        out << "    lea " << reg2(this->windows) << ", [rel input_val]\n";
        out << "    xor eax,eax\n";
        out << "    call scanf\n";
        out << "    mov rax, [rel input_val]\n";
        return true;
    }

    if (nameLower == BUILTIN_KATU) {
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
        return true;
    }

    if (nameLower == BUILTIN_TO_STRING) {
        if (c->getArgs().empty()) return true;
        const Expr *arg = c->getArgs()[0].get();
        if (isStringExpr(arg, locals)) {
            emitExpr(arg, locals);
            return true;
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
            return true;
        }
        emitExpr(arg, locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_to_string\n";
        return true;
    }

    if (nameLower == BUILTIN_TO_NUMBER) {
        if (c->getArgs().empty()) return true;
        const Expr *arg = c->getArgs()[0].get();
        if (isStringExpr(arg, locals)) {
            emitExpr(arg, locals);
            out << "    mov " << reg1(this->windows) << ", rax\n";
            out << "    call aym_to_number\n";
            return true;
        }
        emitExpr(arg, locals);
        return true;
    }

    if (nameLower == BUILTIN_WRITE) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg2(this->windows) << ", rax\n";
        out << "    lea " << reg1(this->windows) << ", [rel fmt_raw]\n";
        out << "    xor eax,eax\n";
        out << "    call printf\n";
        return true;
    }

    return false;
}

} // namespace aym
