#include "codegen_impl.h"
#include "../builtins/builtins.h"

namespace aym {

void CodeGenImpl::emitCallExpr(const CallExpr *c,
                               const std::unordered_map<std::string,int> *locals) {
    std::string nameLower = lowerName(c->getName());
    if (emitBuiltinIoCall(c, locals, nameLower) ||
        emitBuiltinStringCall(c, locals, nameLower) ||
        emitBuiltinCollectionCall(c, locals, nameLower) ||
        emitBuiltinMathCall(c, locals, nameLower) ||
        emitBuiltinSystemCall(c, locals, nameLower) ||
        emitBuiltinFunctionalCall(c, locals, nameLower) ||
        emitBuiltinFsCall(c, locals, nameLower) ||
        emitBuiltinArrayPrimitiveCall(c, locals, nameLower)) {
        return;
    }

    // user function call
    emitCallArgs(c->getArgs(), locals, 0);
    out << "    call " << c->getName() << "\n";
}

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
