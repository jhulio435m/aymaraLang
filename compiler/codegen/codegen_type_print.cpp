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
    if (auto *i = dynamic_cast<const IndexExpr*>(expr)) {
        if (isMapExpr(i->getBase(), locals)) {
            std::string valueIsNumber = genLabel("map_idx_num");
            std::string valueDone = genLabel("map_idx_done");
            emitExpr(i->getIndex(), locals);
            out << "    mov r14, rax\n";
            emitExpr(i->getBase(), locals);
            out << "    mov rbx, rax\n";
            out << "    mov " << reg2(this->windows) << ", r14\n";
            out << "    mov " << reg1(this->windows) << ", rbx\n";
            out << "    call aym_map_value_is_string_key\n";
            out << "    mov r15, rax\n";
            out << "    mov " << reg2(this->windows) << ", r14\n";
            out << "    mov " << reg1(this->windows) << ", rbx\n";
            out << "    call aym_map_get\n";
            out << "    mov r14, rax\n";
            out << "    cmp r15, 0\n";
            out << "    je " << valueIsNumber << "\n";
            out << "    mov " << reg2(this->windows) << ", r14\n";
            out << "    lea " << reg1(this->windows) << ", [rel fmt_raw]\n";
            out << "    xor eax,eax\n";
            out << "    call printf\n";
            out << "    jmp " << valueDone << "\n";
            out << valueIsNumber << ":\n";
            out << "    mov " << reg2(this->windows) << ", r14\n";
            out << "    lea " << reg1(this->windows) << ", [rel fmt_int_raw]\n";
            out << "    xor eax,eax\n";
            out << "    call printf\n";
            out << valueDone << ":\n";
            return;
        }
    }
    if (isListExpr(expr, locals)) {
        emitPrintList(expr, locals);
        return;
    }
    if (isMapExpr(expr, locals)) {
        emitPrintMap(expr, locals);
        return;
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
    std::string loop = genLabel("list_loop");
    std::string end = genLabel("list_end");
    std::string elemType = listElementType(expr, locals);
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
    out << "    call aym_map_value_is_string\n";
    out << "    mov r15, rax\n";
    out << "    mov " << reg2(this->windows) << ", r13\n";
    out << "    mov " << reg1(this->windows) << ", rbx\n";
    out << "    call aym_map_value_at\n";
    out << "    mov r14, rax\n";
    out << "    cmp r15, 0\n";
    std::string valueIsNumber = genLabel("map_value_num");
    std::string valueDone = genLabel("map_value_done");
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
