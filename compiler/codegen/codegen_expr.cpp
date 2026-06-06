#include "codegen_impl.h"
#include "../builtins/builtins.h"
#include <algorithm>

namespace aym {

bool CodeGenImpl::tryEvalConstant(const Expr *expr, const std::unordered_map<std::string,int> *locals, long long &outValue, bool &outIsBool) const {
    if (!expr) return false;
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
    if (auto *bin = dynamic_cast<const BinaryExpr*>(expr)) {
        long long lVal = 0, rVal = 0;
        bool lIsBool = false, rIsBool = false;
        if (tryEvalConstant(bin->getLeft(), locals, lVal, lIsBool) &&
            tryEvalConstant(bin->getRight(), locals, rVal, rIsBool)) {
            outIsBool = false;
            switch (bin->getOp()) {
                case '+': outValue = lVal + rVal; return true;
                case '-': outValue = lVal - rVal; return true;
                case '*': outValue = lVal * rVal; return true;
                case '/': 
                    if (rVal != 0) { outValue = lVal / rVal; return true; }
                    break;
                case '%': 
                    if (rVal != 0) { outValue = lVal % rVal; return true; }
                    break;
                case '^': {
                    long long val = 1;
                    long long base = lVal;
                    long long exp = rVal;
                    if (exp < 0) {
                        val = 0;
                    } else {
                        while (exp > 0) {
                            if (exp & 1) val *= base;
                            base *= base;
                            exp >>= 1;
                        }
                    }
                    outValue = val;
                    return true;
                }
                case '&': outValue = (lVal && rVal) ? 1 : 0; outIsBool = true; return true;
                case '|': outValue = (lVal || rVal) ? 1 : 0; outIsBool = true; return true;
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
        out << "    mov " << reg1(this->windows) << ", " << l->getElements().size() << "\n";
        out << "    call aym_array_new\n";
        out << "    mov rbx, rax\n";
        size_t idx = 0;
        for (const auto &elem : l->getElements()) {
            emitExpr(elem.get(), locals);
            std::vector<std::string> regs = paramRegs(this->windows);
            out << "    mov " << regs[2] << ", rax\n";
            out << "    mov " << regs[1] << ", " << idx << "\n";
            out << "    mov " << regs[0] << ", rbx\n";
            out << "    call aym_array_set\n";
            ++idx;
        }
        out << "    mov rax, rbx\n";
        return;
    }
    if (auto *m = dynamic_cast<const MapExpr *>(expr)) {
        out << "    mov " << reg1(this->windows) << ", " << m->getItems().size() << "\n";
        out << "    call aym_map_new\n";
        out << "    mov rbx, rax\n";
        std::vector<std::string> regs = paramRegs(this->windows);
        for (const auto &item : m->getItems()) {
            emitExpr(item.first.get(), locals);
            out << "    mov r14, rax\n";
            emitExpr(item.second.get(), locals);
            out << "    mov r15, rax\n";
            out << "    mov " << regs[3] << ", " << (isStringExpr(item.second.get(), locals) ? 1 : 0) << "\n";
            out << "    mov " << regs[2] << ", r15\n";
            out << "    mov " << regs[1] << ", r14\n";
            out << "    mov " << regs[0] << ", rbx\n";
            out << "    call aym_map_set\n";
        }
        out << "    mov rax, rbx\n";
        return;
    }
    if (auto *i = dynamic_cast<const IndexExpr *>(expr)) {
        emitExpr(i->getIndex(), locals);
        out << "    mov " << reg2(this->windows) << ", rax\n";
        emitExpr(i->getBase(), locals);
        out << "    mov " << reg1(this->windows) << ", rax\n";
        if (isMapExpr(i->getBase(), locals)) {
            out << "    call aym_map_get\n";
        } else {
            out << "    call aym_array_get\n";
        }
        return;
    }
    if (auto *m = dynamic_cast<const MemberExpr *>(expr)) {
        if (!m->getStaticField().empty()) {
            out << "    mov rax, [rel " << m->getStaticField() << "]\n";
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
            out << "    mov rax, [rel " << v->getName() << "]\n";
        }
        return;
    }
    if (auto *inc = dynamic_cast<const IncDecExpr *>(expr)) {
        bool isLocal = locals && locals->count(inc->getName());
        if (isLocal) {
            out << "    mov rax, [rbp-" << locals->at(inc->getName()) << "]\n";
        } else {
            out << "    mov rax, [rel " << inc->getName() << "]\n";
        }
        if (inc->prefix()) {
            if (inc->increment()) out << "    add rax, 1\n";
            else out << "    sub rax, 1\n";
            if (isLocal) {
                out << "    mov [rbp-" << locals->at(inc->getName()) << "], rax\n";
            } else {
                out << "    mov [rel " << inc->getName() << "], rax\n";
            }
        } else {
            out << "    mov rbx, rax\n";
            if (inc->increment()) out << "    add rax, 1\n";
            else out << "    sub rax, 1\n";
            if (isLocal) {
                out << "    mov [rbp-" << locals->at(inc->getName()) << "], rax\n";
            } else {
                out << "    mov [rel " << inc->getName() << "], rax\n";
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
            case '/': out << "    cqo\n    idiv rbx\n"; break;
            case '%': out << "    cqo\n    idiv rbx\n    mov rax, rdx\n"; break;
            case '^': {
                std::string loop = genLabel("pow");
                std::string end = genLabel("powend");
                out << "    mov rcx, rbx\n";
                out << "    mov rbx, rax\n";
                out << "    mov rax,1\n";
                out << loop << ":\n";
                out << "    cmp rcx,0\n";
                out << "    je " << end << "\n";
                out << "    imul rax, rbx\n";
                out << "    dec rcx\n";
                out << "    jmp " << loop << "\n";
                out << end << ":\n";
                break;
            }
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
                    out << "    cmp rax,0\n    sete al\n    movzx rax,al\n";
                } else {
                    out << "    cmp rax, rbx\n    sete al\n    movzx rax,al\n";
                }
                break;
            case 'd':
                if (leftIsString && rightIsString) {
                    out << "    mov " << reg1(this->windows) << ", rax\n";
                    out << "    mov " << reg2(this->windows) << ", rbx\n";
                    out << "    call strcmp\n";
                    out << "    cmp rax,0\n    setne al\n    movzx rax,al\n";
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
                out << "    neg rax\n";
                break;
            default:
                break; // '+' is a no-op
        }
        return true;
    }

    if (auto *t = dynamic_cast<const TernaryExpr *>(expr)) {
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

void CodeGenImpl::emitCallArgs(const std::vector<std::unique_ptr<Expr>> &args,
                               const std::unordered_map<std::string,int> *locals,
                               size_t regStart) {
    std::vector<std::string> regs = paramRegs(this->windows);
    if (regStart >= regs.size() || args.empty()) {
        return;
    }

    size_t count = std::min(args.size(), regs.size() - regStart);
    // On Win64, nested calls can use the 32-byte shadow space at [rsp..rsp+31].
    // Keep staged arguments above that area to avoid them being clobbered.
    size_t shadow = this->windows ? 32 : 0;
    size_t bytes = count * 8;
    size_t frame = (shadow + bytes + 15) & ~static_cast<size_t>(15);
    if (frame > 0) {
        out << "    sub rsp, " << frame << "\n";
    }

    for (size_t i = 0; i < count; ++i) {
        emitExpr(args[i].get(), locals);
        out << "    mov [rsp+" << (shadow + i * 8) << "], rax\n";
    }

    for (size_t i = 0; i < count; ++i) {
        out << "    mov " << regs[regStart + i] << ", [rsp+" << (shadow + i * 8) << "]\n";
    }

    if (frame > 0) {
        out << "    add rsp, " << frame << "\n";
    }
}

} // namespace aym
