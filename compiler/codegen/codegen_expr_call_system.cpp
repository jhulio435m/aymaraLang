#include "codegen_impl.h"
#include "../builtins/builtins.h"

namespace aym {

bool CodeGenImpl::emitBuiltinSystemCall(const CallExpr *c,
                                        const std::unordered_map<std::string,int> *locals,
                                        const std::string &nameLower) {
    if (nameLower == BUILTIN_RANDOM) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_random\n";
        return true;
    }

    if (nameLower == BUILTIN_SLEEP) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_sleep\n";
        return true;
    }

    if (nameLower == BUILTIN_PANTALLA_LIMPIA) {
        out << "    call aym_term_clear\n";
        return true;
    }

    if (nameLower == BUILTIN_CURSOR_MOVER) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_term_move\n";
        return true;
    }

    if (nameLower == BUILTIN_COLOR) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_term_color\n";
        return true;
    }

    if (nameLower == BUILTIN_COLOR_RESTABLECER) {
        out << "    call aym_term_reset\n";
        return true;
    }

    if (nameLower == BUILTIN_CURSOR_VISIBLE) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_term_cursor\n";
        return true;
    }

    if (nameLower == BUILTIN_TECLA) {
        out << "    call aym_key_poll\n";
        return true;
    }

    if (nameLower == BUILTIN_TIEMPO_MS) {
        out << "    call aym_time_ms\n";
        return true;
    }

    if (nameLower == BUILTIN_UJA_QALLTA) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_gfx_open\n";
        return true;
    }

    if (nameLower == BUILTIN_UJA_UTJI) {
        out << "    call aym_gfx_is_open\n";
        return true;
    }

    if (nameLower == BUILTIN_UJA_PICHHA) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_gfx_clear\n";
        return true;
    }

    if (nameLower == BUILTIN_UJA_SUYU) {
        if (this->windows) {
            // Set RGB color (args 4..6) using a Win64 ABI-compliant call.
            out << "    sub rsp, 64\n";
            emitExpr(c->getArgs()[4].get(), locals);
            out << "    mov [rsp+32], rax\n";
            emitExpr(c->getArgs()[5].get(), locals);
            out << "    mov [rsp+40], rax\n";
            emitExpr(c->getArgs()[6].get(), locals);
            out << "    mov [rsp+48], rax\n";
            out << "    mov rcx, [rsp+32]\n";
            out << "    mov rdx, [rsp+40]\n";
            out << "    mov r8,  [rsp+48]\n";
            out << "    call aym_gfx_set_color\n";
            out << "    add rsp, 64\n";

            // Draw rect with x,y,w,h (args 0..3).
            out << "    sub rsp, 64\n";
            emitExpr(c->getArgs()[0].get(), locals);
            out << "    mov [rsp+32], rax\n";
            emitExpr(c->getArgs()[1].get(), locals);
            out << "    mov [rsp+40], rax\n";
            emitExpr(c->getArgs()[2].get(), locals);
            out << "    mov [rsp+48], rax\n";
            emitExpr(c->getArgs()[3].get(), locals);
            out << "    mov [rsp+56], rax\n";
            out << "    mov rcx, [rsp+32]\n";
            out << "    mov rdx, [rsp+40]\n";
            out << "    mov r8,  [rsp+48]\n";
            out << "    mov r9,  [rsp+56]\n";
            out << "    call aym_gfx_rect4\n";
            out << "    add rsp, 64\n";
        } else {
            emitCallArgs(c->getArgs(), locals, 0);
            out << "    call aym_gfx_rect\n";
        }
        return true;
    }

    if (nameLower == BUILTIN_UJA_QILLQA) {
        if (this->windows) {
            // Set RGB color (args 3..5) using a Win64 ABI-compliant call.
            out << "    sub rsp, 64\n";
            emitExpr(c->getArgs()[3].get(), locals);
            out << "    mov [rsp+32], rax\n";
            emitExpr(c->getArgs()[4].get(), locals);
            out << "    mov [rsp+40], rax\n";
            emitExpr(c->getArgs()[5].get(), locals);
            out << "    mov [rsp+48], rax\n";
            out << "    mov rcx, [rsp+32]\n";
            out << "    mov rdx, [rsp+40]\n";
            out << "    mov r8,  [rsp+48]\n";
            out << "    call aym_gfx_set_color\n";
            out << "    add rsp, 64\n";

            // Draw text with text,x,y (args 0..2).
            out << "    sub rsp, 64\n";
            emitExpr(c->getArgs()[0].get(), locals);
            out << "    mov [rsp+32], rax\n";
            emitExpr(c->getArgs()[1].get(), locals);
            out << "    mov [rsp+40], rax\n";
            emitExpr(c->getArgs()[2].get(), locals);
            out << "    mov [rsp+48], rax\n";
            out << "    mov rcx, [rsp+32]\n";
            out << "    mov rdx, [rsp+40]\n";
            out << "    mov r8,  [rsp+48]\n";
            out << "    call aym_gfx_text3\n";
            out << "    add rsp, 64\n";
        } else {
            emitCallArgs(c->getArgs(), locals, 0);
            out << "    call aym_gfx_text\n";
        }
        return true;
    }

    if (nameLower == BUILTIN_UJA_USTAYA) {
        out << "    call aym_gfx_present\n";
        return true;
    }

    if (nameLower == BUILTIN_UJA_TUKUYA) {
        out << "    call aym_gfx_close\n";
        return true;
    }

    if (nameLower == BUILTIN_UJA_TECLA) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_gfx_key_down\n";
        return true;
    }

    if (nameLower == BUILTIN_ARG_CANTIDAD) {
        out << "    call aym_argc\n";
        return true;
    }

    if (nameLower == BUILTIN_ARG_OBTENER) {
        emitExpr(c->getArgs()[0].get(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_argv_get\n";
        return true;
    }

    if (nameLower == BUILTIN_AFIRMA) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_assert\n";
        return true;
    }

    return false;
}

bool CodeGenImpl::emitBuiltinFunctionalCall(const CallExpr *c,
                                            const std::unordered_map<std::string,int> *locals,
                                            const std::string &nameLower) {
    if (nameLower == BUILTIN_MAP || nameLower == BUILTIN_MAYJTAYA) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_hof_map\n";
        return true;
    }

    if (nameLower == BUILTIN_FILTER || nameLower == BUILTIN_AJLLI) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_hof_filter\n";
        return true;
    }

    if (nameLower == BUILTIN_REDUCE || nameLower == BUILTIN_THAQTHAPI) {
        emitCallArgs(c->getArgs(), locals, 0);
        out << "    call aym_hof_reduce\n";
        return true;
    }

    return false;
}

} // namespace aym
