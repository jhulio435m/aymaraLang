#include "codegen_impl.h"
#include "../builtins/builtins.h"
#include "../utils/class_names.h"

namespace aym {

bool CodeGenImpl::emitStmtBasic(const Stmt *stmt,
                               const std::unordered_map<std::string,int> *locals,
                               const std::string &endLabel) {
    if (auto *p = dynamic_cast<const PrintStmt *>(stmt)) {
        const auto &exprs = p->getExprs();
        const Expr *sepExpr = p->getSeparator();
        const Expr *termExpr = p->getTerminator();
        for (size_t i = 0; i < exprs.size(); ++i) {
            emitPrintValue(exprs[i].get(), locals);
            if (i + 1 < exprs.size()) {
                if (sepExpr) emitPrintValue(sepExpr, locals);
                else emitPrintDefault("print_sep");
            }
        }
        if (termExpr) emitPrintValue(termExpr, locals);
        else emitPrintDefault("print_term");
        return true;
    }
    if (auto *e = dynamic_cast<const ExprStmt *>(stmt)) {
        if (e->getExpr()) emitExpr(e->getExpr(), locals);
        return true;
    }
    if (auto *a = dynamic_cast<const AssignStmt *>(stmt)) {
        bool str = false;
        if (locals && currentLocalStrings.count(a->getName())) str = currentLocalStrings[a->getName()];
        else if (!locals && globalTypes.count(a->getName()) && globalTypes[a->getName()] == "aru") str = true;

        if (auto *call = dynamic_cast<const CallExpr*>(a->getValue()); call && call->getName()==BUILTIN_INPUT) {
            if (str)
                emitInput(true);
            else
                emitInput(false);
        } else {
            emitExpr(a->getValue(), locals);
        }
        if (locals && locals->count(a->getName())) {
            out << "    mov [rbp-" << locals->at(a->getName()) << "], rax\n";
        } else {
            out << "    mov [rel " << a->getName() << "], rax\n";
        }
        return true;
    }
    if (auto *a = dynamic_cast<const IndexAssignStmt *>(stmt)) {
        std::vector<std::string> regs = paramRegs(this->windows);
        if (auto *baseVar = dynamic_cast<const VariableExpr*>(a->getBase())) {
            auto classIt = classes.find(baseVar->getName());
            if (classIt != classes.end()) {
                if (auto *indexLit = dynamic_cast<const StringExpr*>(a->getIndex())) {
                    emitExpr(a->getValue(), locals);
                    std::string staticName = classStaticFieldName(baseVar->getName(), indexLit->getValue());
                    out << "    mov [rel " << staticName << "], rax\n";
                    return true;
                }
            }
        }
        emitExpr(a->getValue(), locals);
        out << "    mov r14, rax\n";
        emitExpr(a->getIndex(), locals);
        out << "    mov r15, rax\n";
        emitExpr(a->getBase(), locals);
        out << "    mov rbx, rax\n";
        if (isMapExpr(a->getBase(), locals)) {
            out << "    mov " << regs[3] << ", " << (isStringExpr(a->getValue(), locals) ? 1 : 0) << "\n";
            out << "    mov " << regs[2] << ", r14\n";
            out << "    mov " << regs[1] << ", r15\n";
            out << "    mov " << regs[0] << ", rbx\n";
            out << "    call aym_map_set\n";
        } else {
            out << "    mov " << regs[2] << ", r14\n";
            out << "    mov " << regs[1] << ", r15\n";
            out << "    mov " << regs[0] << ", rbx\n";
            out << "    call aym_array_set\n";
        }
        return true;
    }
    if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt)) {
        if (v->getInit()) {
            bool str = (v->getType() == "aru");
            if (auto *call = dynamic_cast<const CallExpr*>(v->getInit()); call && call->getName()==BUILTIN_INPUT) {
                emitInput(str);
            } else {
                emitExpr(v->getInit(), locals);
            }
            if (locals && locals->count(v->getName())) {
                out << "    mov [rbp-" << locals->at(v->getName()) << "], rax\n";
            } else {
                out << "    mov [rel " << v->getName() << "], rax\n";
            }
        }
        return true;
    }
    if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
        for (const auto &s : b->statements) emitStmt(s.get(), locals, endLabel);
        return true;
    }
    (void)endLabel;
    return false;
}

} // namespace aym

