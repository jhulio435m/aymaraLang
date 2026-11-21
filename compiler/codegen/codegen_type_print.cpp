#include "codegen_impl.h"

namespace aym {

void CodeGenImpl::emitPrintDefault(const std::string &label) {
    out << "    lea " << reg1(this->windows) << ", [rel fmt_raw]\n";
    out << "    lea " << reg2(this->windows) << ", [rel " << label << "]\n";
    out << "    xor eax,eax\n";
    out << "    call printf\n";
}

void CodeGenImpl::emitPrintValue(const Expr *expr,
                                 const std::unordered_map<std::string,int> *locals) {
    if (!expr) return;
    if (isFloatValue(expr)) {
        emitExpr(expr, locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_print_float\n";
        return;
    }
    if (isListExpr(expr, locals)) {
        emitPrintList(expr, locals);
        return;
    }
    if (isMapExpr(expr, locals)) {
        emitPrintMap(expr, locals);
        return;
    }
    if (auto *i = dynamic_cast<const IndexExpr*>(expr)) {
        if (isMapExpr(i->getBase(), locals)) {
            int spillPad = this->windows ? 40 : 8;
            std::string valueIsNumber = genLabel("map_idx_num");
            std::string valueDone = genLabel("map_idx_done");
            emitExpr(i->getBase(), locals);
            out << "    push rax\n";
            out << "    sub rsp, " << spillPad << "\n";
            emitExpr(i->getIndex(), locals);
            out << "    push rax\n";
            out << "    sub rsp, " << spillPad << "\n";
            out << "    mov " << reg2(this->windows) << ", rax\n";
            out << "    mov " << reg1(this->windows) << ", [rsp+" << (2 * spillPad + 8) << "]\n";
            out << "    call aym_map_value_kind_key\n";
            out << "    add rsp, " << spillPad << "\n";
            out << "    pop " << reg2(this->windows) << "\n";
            out << "    add rsp, " << spillPad << "\n";
            out << "    pop " << reg1(this->windows) << "\n";
            out << "    push rax\n";
            out << "    sub rsp, " << spillPad << "\n";
            out << "    call aym_map_get\n";
            out << "    mov " << reg2(this->windows) << ", rax\n";
            out << "    add rsp, " << spillPad << "\n";
            out << "    pop rax\n";
            std::string valueIsString = genLabel("map_idx_str");
            out << "    cmp rax, 2\n";
            out << "    jne " << valueIsString << "\n";
            out << "    mov " << reg1(this->windows) << ", " << reg2(this->windows) << "\n";
            out << "    call aym_print_float\n";
            out << "    jmp " << valueDone << "\n";
            out << valueIsString << ":\n";
            out << "    cmp rax, 0\n";
            out << "    je " << valueIsNumber << "\n";
            out << "    lea " << reg1(this->windows) << ", [rel fmt_raw]\n";
            out << "    xor eax,eax\n";
            out << "    call printf\n";
            out << "    jmp " << valueDone << "\n";
            out << valueIsNumber << ":\n";
            out << "    lea " << reg1(this->windows) << ", [rel fmt_int_raw]\n";
            out << "    xor eax,eax\n";
            out << "    call printf\n";
            out << valueDone << ":\n";
            return;
        }
    }
    if (isBoolExpr(expr, locals)) {
        std::string falseLbl = genLabel("bool_false");
        std::string endLbl = genLabel("bool_end");
        emitExpr(expr, locals);
        out << "    cmp rax,0\n";
        out << "    je " << falseLbl << "\n";
        emitPrintDefault("bool_true");
        out << "    jmp " << endLbl << "\n";
        out << falseLbl << ":\n";
        emitPrintDefault("bool_false");
        out << endLbl << ":\n";
        return;
    }
    emitExpr(expr, locals);
    if (isStringExpr(expr, locals)) {
        out << "    mov " << reg2(this->windows) << ", rax\n";
        out << "    lea " << reg1(this->windows) << ", [rel fmt_raw]\n";
    } else {
        out << "    mov " << reg2(this->windows) << ", rax\n";
        out << "    lea " << reg1(this->windows) << ", [rel fmt_int_raw]\n";
    }
    out << "    xor eax,eax\n";
    out << "    call printf\n";
}

void CodeGenImpl::emitPrintList(const Expr *expr,
                                const std::unordered_map<std::string,int> *locals) {
    std::string elemType = listElementType(expr, locals);
    {
        // Listas de chikata (a cualquier profundidad): el tipo semantico es
        // fiable; el runtime imprime cada double.
        std::string st = expr->getSemType();
        long depth = 0;
        while (st.rfind("t'aqa:", 0) == 0) {
            st = st.substr(6);
            ++depth;
        }
        if (depth > 0 && st == "chikata") {
            std::vector<std::string> regs = paramRegs(this->windows);
            emitExpr(expr, locals);
            out << "    mov " << regs[0] << ", rax\n";
            out << "    mov " << regs[1] << ", " << depth << "\n";
            out << "    mov " << regs[2] << ", 2\n";
            out << "    call aym_print_list_depth\n";
            return;
        }
    }
    if (elemType.rfind("t'aqa", 0) == 0) {
        std::string t = elemType;
        long depth = 1;
        while (t.rfind("t'aqa:", 0) == 0) {
            t = t.substr(6);
            ++depth;
        }
        if (t == "t'aqa") {
            ++depth;
        }
        long elemIsStr = (t == "aru") ? 1 : 0;
        std::vector<std::string> regs = paramRegs(this->windows);
        emitExpr(expr, locals);
        out << "    mov " << regs[0] << ", rax\n";
        out << "    mov " << regs[1] << ", " << depth << "\n";
        out << "    mov " << regs[2] << ", " << elemIsStr << "\n";
        out << "    call aym_print_list_depth\n";
        return;
    }
    std::string loop = genLabel("list_loop");
    std::string end = genLabel("list_end");
    emitExpr(expr, locals);
    out << "    mov rbx, rax\n";
    emitPrintDefault("list_open");
    out << "    mov " << reg1(this->windows) << ", rbx\n";
    out << "    call aym_array_length\n";
    out << "    mov r12, rax\n";
    out << "    xor r13, r13\n";
    out << loop << ":\n";
    out << "    cmp r13, r12\n";
    out << "    je " << end << "\n";
    out << "    mov " << reg2(this->windows) << ", r13\n";
    out << "    mov " << reg1(this->windows) << ", rbx\n";
    out << "    call aym_array_get\n";
    if (elemType == "aru") {
        out << "    mov r14, rax\n";
        emitPrintDefault("list_quote");
        out << "    mov " << reg2(this->windows) << ", r14\n";
        out << "    lea " << reg1(this->windows) << ", [rel fmt_raw]\n";
    } else {
        out << "    mov " << reg2(this->windows) << ", rax\n";
        out << "    lea " << reg1(this->windows) << ", [rel fmt_int_raw]\n";
    }
    out << "    xor eax,eax\n";
    out << "    call printf\n";
    if (elemType == "aru") {
        emitPrintDefault("list_quote");
    }
    out << "    inc r13\n";
    out << "    cmp r13, r12\n";
    out << "    je " << end << "\n";
    emitPrintDefault("list_sep");
    out << "    jmp " << loop << "\n";
    out << end << ":\n";
    emitPrintDefault("list_close");
}

void CodeGenImpl::emitPrintMap(const Expr *expr,
                               const std::unordered_map<std::string,int> *locals) {
    std::string loop = genLabel("map_loop");
    std::string end = genLabel("map_end");
    emitExpr(expr, locals);
    out << "    mov rbx, rax\n";
    emitPrintDefault("map_open");
    out << "    mov " << reg1(this->windows) << ", rbx\n";
    out << "    call aym_map_size\n";
    out << "    mov r12, rax\n";
    out << "    xor r13, r13\n";
    out << loop << ":\n";
    out << "    cmp r13, r12\n";
    out << "    je " << end << "\n";
    out << "    mov " << reg2(this->windows) << ", r13\n";
    out << "    mov " << reg1(this->windows) << ", rbx\n";
    out << "    call aym_map_key_at\n";
    out << "    mov r14, rax\n";
    out << "    mov " << reg2(this->windows) << ", r14\n";
    out << "    lea " << reg1(this->windows) << ", [rel fmt_raw]\n";
    out << "    xor eax,eax\n";
    out << "    call printf\n";
    emitPrintDefault("map_colon");
    out << "    mov " << reg2(this->windows) << ", r13\n";
    out << "    mov " << reg1(this->windows) << ", rbx\n";
    out << "    call aym_map_value_kind\n";
    out << "    mov r15, rax\n";
    out << "    mov " << reg2(this->windows) << ", r13\n";
    out << "    mov " << reg1(this->windows) << ", rbx\n";
    out << "    call aym_map_value_at\n";
    out << "    mov r14, rax\n";
    std::string valueIsNumber = genLabel("map_value_num");
    std::string valueDone = genLabel("map_value_done");
    std::string valueNotFloat = genLabel("map_value_nf");
    out << "    cmp r15, 2\n";
    out << "    jne " << valueNotFloat << "\n";
    out << "    mov " << reg1(this->windows) << ", r14\n";
    out << "    call aym_print_float\n";
    out << "    jmp " << valueDone << "\n";
    out << valueNotFloat << ":\n";
    out << "    cmp r15, 0\n";
    out << "    je " << valueIsNumber << "\n";
    emitPrintDefault("list_quote");
    out << "    mov " << reg2(this->windows) << ", r14\n";
    out << "    lea " << reg1(this->windows) << ", [rel fmt_raw]\n";
    out << "    xor eax,eax\n";
    out << "    call printf\n";
    emitPrintDefault("list_quote");
    out << "    jmp " << valueDone << "\n";
    out << valueIsNumber << ":\n";
    out << "    mov " << reg2(this->windows) << ", r14\n";
    out << "    lea " << reg1(this->windows) << ", [rel fmt_int_raw]\n";
    out << "    xor eax,eax\n";
    out << "    call printf\n";
    out << valueDone << ":\n";
    out << "    inc r13\n";
    out << "    cmp r13, r12\n";
    out << "    je " << end << "\n";
    emitPrintDefault("map_sep");
    out << "    jmp " << loop << "\n";
    out << end << ":\n";
    emitPrintDefault("map_close");
}

} // namespace aym
