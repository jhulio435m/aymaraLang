#include "codegen_impl.h"
#include "../builtins/builtins.h"
#include <algorithm>
#include <limits>
#include <cstring>

namespace aym {

bool CodeGenImpl::tryEvalConstant(const Expr *expr, const std::unordered_map<std::string,int> *locals, long long &outValue, bool &outIsBool) const {
    if (!expr) return false;
    // El plegado es solo entero. Un entero marcado para conversion si se
    // pliega: emitExpr aplica la conversion despues. Un padre con un hijo
    // convertido siempre tiene un hermano chikata, que no se pliega.
    if (expr->getSemType() == "chikata") return false;
    if (auto *n = dynamic_cast<const NumberExpr*>(expr)) {
        outValue = n->getValue();
        outIsBool = false;
        return true;
    }
    if (auto *b = dynamic_cast<const BoolExpr*>(expr)) {
        outValue = b->getValue() ? 1 : 0;
        outIsBool = true;
        return true;
    }
    if (auto *u = dynamic_cast<const UnaryExpr*>(expr)) {
        long long val = 0;
        bool valIsBool = false;
        if (tryEvalConstant(u->getExpr(), locals, val, valIsBool)) {
            switch (u->getOp()) {
                case '-':
                    outValue = static_cast<long long>(-static_cast<unsigned long long>(val));
                    outIsBool = false;
                    return true;
                case '+':
                    outValue = val;
                    outIsBool = valIsBool;
                    return true;
                case '!':
                    outValue = (val == 0) ? 1 : 0;
                    outIsBool = true;
                    return true;
                case '~':
                    outValue = static_cast<long long>(~static_cast<unsigned long long>(val));
                    outIsBool = false;
                    return true;
                default:
                    break;
            }
        }
        return false;
    }
    if (auto *t = dynamic_cast<const TernaryExpr*>(expr)) {
        long long condVal = 0;
        bool condIsBool = false;
        if (tryEvalConstant(t->getCondition(), locals, condVal, condIsBool)) {
            const Expr *taken = (condVal != 0) ? t->getThen() : t->getElse();
            return tryEvalConstant(taken, locals, outValue, outIsBool);
        }
        return false;
    }
    if (auto *bin = dynamic_cast<const BinaryExpr*>(expr)) {
        long long lVal = 0, rVal = 0;
        bool lIsBool = false, rIsBool = false;
        if (!tryEvalConstant(bin->getLeft(), locals, lVal, lIsBool)) {
            return false;
        }
        if (bin->getOp() == '&') {
            if (lVal == 0) {
                outValue = 0;
                outIsBool = true;
                return true;
            }
            if (tryEvalConstant(bin->getRight(), locals, rVal, rIsBool)) {
                outValue = (rVal != 0) ? 1 : 0;
                outIsBool = true;
                return true;
            }
            return false;
        }
        if (bin->getOp() == '|') {
            if (lVal != 0) {
                outValue = 1;
                outIsBool = true;
                return true;
            }
            if (tryEvalConstant(bin->getRight(), locals, rVal, rIsBool)) {
                outValue = (rVal != 0) ? 1 : 0;
                outIsBool = true;
                return true;
            }
            return false;
        }
        if (tryEvalConstant(bin->getRight(), locals, rVal, rIsBool)) {
            outIsBool = false;
            const unsigned long long lu = static_cast<unsigned long long>(lVal);
            const unsigned long long ru = static_cast<unsigned long long>(rVal);
            const long long llMin = std::numeric_limits<long long>::min();
            switch (bin->getOp()) {
                case '+': outValue = static_cast<long long>(lu + ru); return true;
                case '-': outValue = static_cast<long long>(lu - ru); return true;
                case '*': outValue = static_cast<long long>(lu * ru); return true;
                case '/':
                    if (rVal != 0 && !(lVal == llMin && rVal == -1)) {
                        outValue = lVal / rVal;
                        return true;
                    }
                    break;
                case '%':
                    if (rVal != 0 && !(lVal == llMin && rVal == -1)) {
                        outValue = lVal % rVal;
                        return true;
                    }
                    break;
                case '^': {
                    unsigned long long val = 1;
                    unsigned long long base = lu;
                    long long exp = rVal;
                    if (exp < 0) {
                        // Misma semantica que aym_ipow: 1/b^n truncado.
                        if (lVal == 0) break; // division por cero: en ejecucion
                        if (lVal == 1) val = 1;
                        else if (lVal == -1) val = (exp & 1) ? static_cast<unsigned long long>(-1LL) : 1;
                        else val = 0;
                    } else {
                        while (exp > 0) {
                            if (exp & 1) val *= base;
                            base *= base;
                            exp >>= 1;
                        }
                    }
                    outValue = static_cast<long long>(val);
                    return true;
                }
                case 'A': outValue = static_cast<long long>(lu & ru); return true;
                case 'O': outValue = static_cast<long long>(lu | ru); return true;
                case 'X': outValue = static_cast<long long>(lu ^ ru); return true;
                case 'L':
                    if (rVal >= 0 && rVal < 64) {
                        outValue = static_cast<long long>(lu << rVal);
                        return true;
                    }
                    break;
                case 'R':
                    if (rVal >= 0 && rVal < 64) {
                        outValue = lVal >> rVal;
                        return true;
                    }
                    break;
                case 's': outValue = (lVal == rVal) ? 1 : 0; outIsBool = true; return true;
                case 'd': outValue = (lVal != rVal) ? 1 : 0; outIsBool = true; return true;
                case '<': outValue = (lVal < rVal) ? 1 : 0; outIsBool = true; return true;
                case 'l': outValue = (lVal <= rVal) ? 1 : 0; outIsBool = true; return true;
                case '>': outValue = (lVal > rVal) ? 1 : 0; outIsBool = true; return true;
                case 'g': outValue = (lVal >= rVal) ? 1 : 0; outIsBool = true; return true;
            }
        }
    }
    return false;
}

void CodeGenImpl::emitExpr(const Expr *expr,
                           const std::unordered_map<std::string,int> *locals) {
    if (!expr) return;
    emitExprInner(expr, locals);
    if (expr->getConvertTo() == "chikata" && expr->getSemType() == "jakhüwi") {
        out << "    cvtsi2sd xmm0, rax\n";
        out << "    movq rax, xmm0\n";
    }
}

void CodeGenImpl::emitExprInner(const Expr *expr,
                                const std::unordered_map<std::string,int> *locals) {
    if (!expr) return;
    if (auto *f = dynamic_cast<const FloatExpr *>(expr)) {
        double v = f->getValue();
        unsigned long long bits = 0;
        std::memcpy(&bits, &v, sizeof(bits));
        out << "    mov rax, 0x" << std::hex << bits << std::dec << "\n";
        return;
    }
    long long constVal = 0;
    bool isBool = false;
    if (tryEvalConstant(expr, locals, constVal, isBool)) {
        out << "    mov rax, " << constVal << "\n";
        return;
    }

    if (auto *s = dynamic_cast<const StringExpr *>(expr)) {
        size_t idx = findString(s->getValue());
        out << "    lea rax, [rel str" << idx << "]\n";
        return;
    }
    if (auto *l = dynamic_cast<const ListExpr *>(expr)) {
        std::vector<std::string> regs = paramRegs(this->windows);
        out << "    mov " << regs[0] << ", " << l->getElements().size() << "\n";
        out << "    call aym_array_new\n";
        if (l->getElements().empty()) {
            return;
        }
        int spillPad = this->windows ? 40 : 8;
        out << "    push rax\n";
        out << "    sub rsp, " << spillPad << "\n";
        size_t idx = 0;
        for (const auto &elem : l->getElements()) {
            emitExpr(elem.get(), locals);
            out << "    mov " << regs[2] << ", rax\n";
            out << "    mov " << regs[1] << ", " << idx << "\n";
            out << "    mov " << regs[0] << ", [rsp+" << spillPad << "]\n";
            out << "    call aym_array_set\n";
            ++idx;
        }
        out << "    add rsp, " << spillPad << "\n";
        out << "    pop rax\n";
        return;
    }
    if (auto *m = dynamic_cast<const MapExpr *>(expr)) {
        std::vector<std::string> regs = paramRegs(this->windows);
        out << "    mov " << regs[0] << ", " << m->getItems().size() << "\n";
        out << "    call aym_map_new\n";
        if (m->getItems().empty()) {
            return;
        }
        int spillPad = this->windows ? 40 : 8;
        out << "    push rax\n";
        out << "    sub rsp, " << spillPad << "\n";
        for (const auto &item : m->getItems()) {
            emitExpr(item.first.get(), locals);
            out << "    push rax\n";
            out << "    sub rsp, " << spillPad << "\n";
            emitExpr(item.second.get(), locals);
            out << "    mov " << regs[2] << ", rax\n";
            out << "    mov " << regs[3] << ", " << (isStringExpr(item.second.get(), locals) ? 1 : (isFloatValue(item.second.get()) ? 2 : 0)) << "\n";
            out << "    add rsp, " << spillPad << "\n";
            out << "    pop " << regs[1] << "\n";
            out << "    mov " << regs[0] << ", [rsp+" << spillPad << "]\n";
            out << "    call aym_map_set\n";
        }
        out << "    add rsp, " << spillPad << "\n";
        out << "    pop rax\n";
        return;
    }
    if (auto *i = dynamic_cast<const IndexExpr *>(expr)) {
        int spillPad = this->windows ? 40 : 8;
        emitExpr(i->getBase(), locals);
        out << "    push rax\n";
        out << "    sub rsp, " << spillPad << "\n";
        emitExpr(i->getIndex(), locals);
        out << "    mov " << reg2(this->windows) << ", rax\n";
        out << "    add rsp, " << spillPad << "\n";
        out << "    pop " << reg1(this->windows) << "\n";
        if (isMapExpr(i->getBase(), locals)) {
            out << "    call aym_map_get\n";
        } else if (isStringExpr(i->getBase(), locals)) {
            out << "    call aym_str_char_at\n";
        } else {
            out << "    call aym_array_get\n";
        }
        return;
    }
    if (auto *m = dynamic_cast<const MemberExpr *>(expr)) {
        if (!m->getStaticField().empty()) {
            out << "    mov rax, [rel $" << m->getStaticField() << "]\n";
            return;
        }
        emitExpr(m->getBase(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        if (m->isExceptionAccess()) {
            if (m->getMember() == "suti") {
                out << "    call aym_exception_type\n";
            } else if (m->getMember() == "aru") {
                out << "    call aym_exception_message\n";
            } else {
                out << "    mov rax, 0\n";
            }
        } else {
            size_t keyIdx = findString(m->getMember());
            out << "    lea " << reg2(this->windows) << ", [rel str" << keyIdx << "]\n";
            out << "    call aym_map_get\n";
        }
        return;
    }
    if (auto *v = dynamic_cast<const VariableExpr *>(expr)) {
        if (locals && locals->count(v->getName())) {
            out << "    mov rax, [rbp-" << locals->at(v->getName()) << "]\n";
        } else {
            out << "    mov rax, [rel $" << v->getName() << "]\n";
        }
        return;
    }
    if (auto *inc = dynamic_cast<const IncDecExpr *>(expr)) {
        bool isLocal = locals && locals->count(inc->getName());
        if (isLocal) {
            out << "    mov rax, [rbp-" << locals->at(inc->getName()) << "]\n";
        } else {
            out << "    mov rax, [rel $" << inc->getName() << "]\n";
        }
        auto step = [&]() {
            if (inc->getSemType() == "chikata") {
                out << "    movq xmm0, rax\n";
                out << "    mov rcx, 0x3ff0000000000000\n"; // 1.0
                out << "    movq xmm1, rcx\n";
                out << (inc->increment() ? "    addsd xmm0, xmm1\n" : "    subsd xmm0, xmm1\n");
                out << "    movq rax, xmm0\n";
            } else if (inc->increment()) {
                out << "    add rax, 1\n";
            } else {
                out << "    sub rax, 1\n";
            }
        };
        if (inc->prefix()) {
            step();
            if (isLocal) {
                out << "    mov [rbp-" << locals->at(inc->getName()) << "], rax\n";
            } else {
                out << "    mov [rel $" << inc->getName() << "], rax\n";
            }
        } else {
            out << "    mov rbx, rax\n";
            step();
            if (isLocal) {
                out << "    mov [rbp-" << locals->at(inc->getName()) << "], rax\n";
            } else {
                out << "    mov [rel $" << inc->getName() << "], rax\n";
            }
            out << "    mov rax, rbx\n";
        }
        return;
    }
    if (emitExprOperator(expr, locals)) {
        return;
    }
    if (auto *n = dynamic_cast<const NewExpr *>(expr)) {
        if (emitNewExpr(n, locals)) {
            return;
        }
    }
    if (auto *m = dynamic_cast<const MemberCallExpr *>(expr)) {
        if (emitMemberCallExpr(m, locals)) {
            return;
        }
    }
    if (auto *f = dynamic_cast<const FunctionRefExpr *>(expr)) {
        out << "    lea rax, [rel " << f->getName() << "]\n";
        return;
    }
    if (auto *s = dynamic_cast<const SuperExpr *>(expr)) {
        if (emitSuperExpr(s, locals)) {
            return;
        }
    }
    if (auto *c = dynamic_cast<const CallExpr *>(expr)) {
        emitCallExpr(c, locals);
        return;
    }
}

bool CodeGenImpl::emitExprOperator(const Expr *expr,
                                   const std::unordered_map<std::string,int> *locals) {
    if (auto *b = dynamic_cast<const BinaryExpr *>(expr)) {
        if (b->getOp() == '&') {
            std::string falseLbl = genLabel("and_false");
            std::string endLbl = genLabel("and_end");
            emitExpr(b->getLeft(), locals);
            out << "    cmp rax,0\n";
            out << "    je " << falseLbl << "\n";
            emitExpr(b->getRight(), locals);
            out << "    cmp rax,0\n";
            out << "    setne al\n";
            out << "    movzx rax,al\n";
            out << "    jmp " << endLbl << "\n";
            out << falseLbl << ":\n";
            out << "    mov rax,0\n";
            out << endLbl << ":\n";
            return true;
        }
        if (b->getOp() == '|') {
            std::string trueLbl = genLabel("or_true");
            std::string endLbl = genLabel("or_end");
            emitExpr(b->getLeft(), locals);
            out << "    cmp rax,0\n";
            out << "    jne " << trueLbl << "\n";
            emitExpr(b->getRight(), locals);
            out << "    cmp rax,0\n";
            out << "    setne al\n";
            out << "    movzx rax,al\n";
            out << "    jmp " << endLbl << "\n";
            out << trueLbl << ":\n";
            out << "    mov rax,1\n";
            out << endLbl << ":\n";
            return true;
        }
        emitExpr(b->getLeft(), locals);
        out << "    push rax\n";
        int spillPad = this->windows ? 40 : 8;
        out << "    sub rsp, " << spillPad << "\n";
        emitExpr(b->getRight(), locals);
        out << "    add rsp, " << spillPad << "\n";
        out << "    mov rbx, rax\n";
        out << "    pop rax\n";
        bool leftIsString = isStringExpr(b->getLeft(), locals);
        bool rightIsString = isStringExpr(b->getRight(), locals);
        if (isFloatValue(b->getLeft()) || isFloatValue(b->getRight())) {
            emitFloatBinary(b->getOp());
            return true;
        }
        if (leftIsString && rightIsString &&
            (b->getOp() == '<' || b->getOp() == 'l' || b->getOp() == '>' || b->getOp() == 'g')) {
            // Orden lexicografico por bytes (UTF-8 conserva el orden de codigo).
            out << "    mov " << reg1(this->windows) << ", rax\n";
            out << "    mov " << reg2(this->windows) << ", rbx\n";
            out << "    call strcmp\n";
            out << "    test eax, eax\n";
            const char *setcc = b->getOp() == '<' ? "setl" : b->getOp() == 'l' ? "setle"
                              : b->getOp() == '>' ? "setg" : "setge";
            out << "    " << setcc << " al\n    movzx rax,al\n";
            return true;
        }
        switch (b->getOp()) {
            case '+':
                if (leftIsString && rightIsString) {
                    out << "    mov " << reg1(this->windows) << ", rax\n";
                    out << "    mov " << reg2(this->windows) << ", rbx\n";
                    out << "    call aym_str_concat\n";
                } else {
                    out << "    add rax, rbx\n";
                }
                break;
            case '-': out << "    sub rax, rbx\n"; break;
            case '*': out << "    imul rax, rbx\n"; break;
            case '/':
            case '%': {
                std::string okLbl = genLabel("div_ok");
                out << "    test rbx, rbx\n    jnz " << okLbl << "\n";
                out << "    call aym_div_zero\n";
                out << okLbl << ":\n";
                // INT_MIN / -1 desborda y 'idiv' lanza #DE (SIGFPE). Con
                // divisor -1 el resultado es -a (con envoltura) y el resto 0.
                std::string normLbl = genLabel("div_norm");
                std::string endLbl = genLabel("div_end");
                out << "    cmp rbx, -1\n    jne " << normLbl << "\n";
                if (b->getOp() == '%') out << "    xor eax, eax\n";
                else out << "    neg rax\n";
                out << "    jmp " << endLbl << "\n";
                out << normLbl << ":\n";
                out << "    cqo\n    idiv rbx\n";
                if (b->getOp() == '%') out << "    mov rax, rdx\n";
                out << endLbl << ":\n";
                break;
            }
            case '^': {
                // Potencia entera por cuadrados en el runtime: O(log n) y con
                // semantica definida para exponentes negativos.
                out << "    mov " << reg1(this->windows) << ", rax\n";
                out << "    mov " << reg2(this->windows) << ", rbx\n";
                out << "    call aym_ipow\n";
                break;
            }
            case 'A':
                out << "    and rax, rbx\n";
                break;
            case 'O':
                out << "    or rax, rbx\n";
                break;
            case 'X':
                out << "    xor rax, rbx\n";
                break;
            case 'L':
                out << "    mov rcx, rbx\n";
                out << "    shl rax, cl\n";
                break;
            case 'R':
                out << "    mov rcx, rbx\n";
                out << "    sar rax, cl\n";
                break;
            case '<':
                out << "    cmp rax, rbx\n    setl al\n    movzx rax,al\n";
                break;
            case 'l':
                out << "    cmp rax, rbx\n    setle al\n    movzx rax,al\n";
                break;
            case '>':
                out << "    cmp rax, rbx\n    setg al\n    movzx rax,al\n";
                break;
            case 'g':
                out << "    cmp rax, rbx\n    setge al\n    movzx rax,al\n";
                break;
            case 's':
                if (leftIsString && rightIsString) {
                    out << "    mov " << reg1(this->windows) << ", rax\n";
                    out << "    mov " << reg2(this->windows) << ", rbx\n";
                    out << "    call strcmp\n";
                    out << "    test eax, eax\n    sete al\n    movzx rax,al\n";
                } else {
                    out << "    cmp rax, rbx\n    sete al\n    movzx rax,al\n";
                }
                break;
            case 'd':
                if (leftIsString && rightIsString) {
                    out << "    mov " << reg1(this->windows) << ", rax\n";
                    out << "    mov " << reg2(this->windows) << ", rbx\n";
                    out << "    call strcmp\n";
                    out << "    test eax, eax\n    setne al\n    movzx rax,al\n";
                } else {
                    out << "    cmp rax, rbx\n    setne al\n    movzx rax,al\n";
                }
                break;
        }
        return true;
    }

    if (auto *u = dynamic_cast<const UnaryExpr *>(expr)) {
        emitExpr(u->getExpr(), locals);
        switch (u->getOp()) {
            case '!':
                out << "    cmp rax,0\n";
                out << "    sete al\n";
                out << "    movzx rax,al\n";
                break;
            case '-':
                if (u->getSemType() == "chikata") {
                    out << "    btc rax, 63\n"; // invierte el signo del double
                } else {
                    out << "    neg rax\n";
                }
                break;
            case '~':
                out << "    not rax\n";
                break;
            default:
                break; // '+' is a no-op
        }
        return true;
    }

    if (auto *t = dynamic_cast<const TernaryExpr *>(expr)) {
        long long condVal = 0;
        bool condIsBool = false;
        if (tryEvalConstant(t->getCondition(), locals, condVal, condIsBool)) {
            emitExpr((condVal != 0) ? t->getThen() : t->getElse(), locals);
            return true;
        }
        std::string elseLbl = genLabel("tern_else");
        std::string endLbl = genLabel("tern_end");
        emitExpr(t->getCondition(), locals);
        out << "    cmp rax,0\n";
        out << "    je " << elseLbl << "\n";
        emitExpr(t->getThen(), locals);
        out << "    jmp " << endLbl << "\n";
        out << elseLbl << ":\n";
        emitExpr(t->getElse(), locals);
        out << endLbl << ":\n";
        return true;
    }

    return false;
}

size_t CodeGenImpl::emitCallArgs(const std::vector<std::unique_ptr<Expr>> &args,
                                 const std::unordered_map<std::string,int> *locals,
                                 size_t regStart) {
    std::vector<std::string> regs = paramRegs(this->windows);
    if (regStart >= regs.size() || args.empty()) {
        return 0;
    }

    size_t count = std::min(args.size(), regs.size() - regStart);
    size_t extra = args.size() - count;
    // On Win64, nested calls can use the 32-byte shadow space at [rsp..rsp+31].
    // Keep staged arguments above that area to avoid them being clobbered.
    size_t shadow = this->windows ? 32 : 0;

    // Argumentos que no caben en registros: van en la pila del llamador, en el
    // orden del ABI ([rsp+shadow] = primero extra al momento del 'call').
    // Se reserva un area alineada a 16 que queda viva hasta despues del call.
    size_t stackArea = 0;
    if (extra > 0) {
        stackArea = (shadow + extra * 8 + 15) & ~static_cast<size_t>(15);
        out << "    sub rsp, " << stackArea << "\n";
    }

    size_t total = count + extra;
    size_t bytes = total * 8;
    size_t frame = (shadow + bytes + 15) & ~static_cast<size_t>(15);
    if (frame > 0) {
        out << "    sub rsp, " << frame << "\n";
    }

    for (size_t i = 0; i < total; ++i) {
        emitExpr(args[i].get(), locals);
        out << "    mov [rsp+" << (shadow + i * 8) << "], rax\n";
    }

    for (size_t j = 0; j < extra; ++j) {
        out << "    mov rax, [rsp+" << (shadow + (count + j) * 8) << "]\n";
        out << "    mov [rsp+" << (frame + shadow + j * 8) << "], rax\n";
    }

    for (size_t i = 0; i < count; ++i) {
        out << "    mov " << regs[regStart + i] << ", [rsp+" << (shadow + i * 8) << "]\n";
    }

    if (frame > 0) {
        out << "    add rsp, " << frame << "\n";
    }
    pendingStackArgs += stackArea;
    return stackArea;
}

void CodeGenImpl::emitFloatBinary(char op) {
    // rax = izquierda, rbx = derecha (bits de double, ya convertidos).
    out << "    movq xmm0, rax\n";
    out << "    movq xmm1, rbx\n";
    switch (op) {
        case '+': out << "    addsd xmm0, xmm1\n    movq rax, xmm0\n"; return;
        case '-': out << "    subsd xmm0, xmm1\n    movq rax, xmm0\n"; return;
        case '*': out << "    mulsd xmm0, xmm1\n    movq rax, xmm0\n"; return;
        case '/': out << "    divsd xmm0, xmm1\n    movq rax, xmm0\n"; return; // IEEE: x/0 = inf
        case '%':
            out << "    mov " << reg1(this->windows) << ", rax\n";
            out << "    mov " << reg2(this->windows) << ", rbx\n";
            out << "    call aym_fmod\n";
            return;
        case '^':
            out << "    mov " << reg1(this->windows) << ", rax\n";
            out << "    mov " << reg2(this->windows) << ", rbx\n";
            out << "    call aym_fpow\n";
            return;
        // Comparaciones: con NaN todas son falsas salvo '!='.
        case '<': out << "    ucomisd xmm1, xmm0\n    seta al\n"; break;
        case 'l': out << "    ucomisd xmm1, xmm0\n    setae al\n"; break;
        case '>': out << "    ucomisd xmm0, xmm1\n    seta al\n"; break;
        case 'g': out << "    ucomisd xmm0, xmm1\n    setae al\n"; break;
        case 's': out << "    ucomisd xmm0, xmm1\n    sete al\n    setnp cl\n    and al, cl\n"; break;
        case 'd': out << "    ucomisd xmm0, xmm1\n    setne al\n    setp cl\n    or al, cl\n"; break;
        default:
            out << "    xor eax, eax\n";
            return;
    }
    out << "    movzx rax, al\n";
}

} // namespace aym
