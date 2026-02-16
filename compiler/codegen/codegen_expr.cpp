#include "codegen_impl.h"
#include "../builtins/builtins.h"

namespace aym {

void CodeGenImpl::emitExpr(const Expr *expr,
                           const std::unordered_map<std::string,int> *locals) {
    if (!expr) return;
    if (auto *n = dynamic_cast<const NumberExpr *>(expr)) {
        out << "    mov rax, " << n->getValue() << "\n";
        return;
    }
    if (auto *b = dynamic_cast<const BoolExpr *>(expr)) {
        out << "    mov rax, " << (b->getValue() ? 1 : 0) << "\n";
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
} // namespace aym
