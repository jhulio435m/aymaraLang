#include "codegen_impl.h"

namespace aym {

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

} // namespace aym
