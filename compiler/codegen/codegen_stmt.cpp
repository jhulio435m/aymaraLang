#include "codegen_impl.h"
#include "../builtins/builtins.h"
#include "../utils/class_names.h"
#include <cstdint>

namespace aym {

void CodeGenImpl::emitStmt(const Stmt *stmt,
                           const std::unordered_map<std::string,int> *locals,
                           const std::string &endLabel) {
    if (!stmt) return;
    if (emitStmtBasic(stmt, locals, endLabel)) return;
    if (emitStmtControl(stmt, locals, endLabel)) return;
    if (emitStmtException(stmt, locals, endLabel)) return;
}

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

bool CodeGenImpl::emitStmtControl(const Stmt *stmt,
                                 const std::unordered_map<std::string,int> *locals,
                                 const std::string &endLabel) {
    if (auto *i = dynamic_cast<const IfStmt *>(stmt)) {
        std::string elseLbl = genLabel("else");
        std::string end = genLabel("endif");
        emitExpr(i->getCondition(), locals);
        out << "    cmp rax,0\n";
        if (i->getElse()) {
            out << "    je " << elseLbl << "\n";
            emitStmt(i->getThen(), locals, endLabel);
            out << "    jmp " << end << "\n";
            out << elseLbl << ":\n";
            emitStmt(i->getElse(), locals, endLabel);
        } else {
            out << "    je " << end << "\n";
            emitStmt(i->getThen(), locals, endLabel);
        }
        out << end << ":\n";
        return true;
    }
    if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) {
        std::string loop = genLabel("loop");
        std::string cont = genLabel("cont");
        std::string end = genLabel("endloop");
        breakLabels.push_back(end);
        continueLabels.push_back(cont);
        loopFinallyDepth.push_back(finallyStack.size());
        out << loop << ":\n";
        if (w->getCondition()) {
            emitExpr(w->getCondition(), locals);
            out << "    cmp rax,0\n";
            out << "    je " << end << "\n";
        }
        emitStmt(w->getBody(), locals, endLabel);
        out << cont << ":\n";
        out << "    jmp " << loop << "\n";
        out << end << ":\n";
        breakLabels.pop_back();
        continueLabels.pop_back();
        loopFinallyDepth.pop_back();
        return true;
    }
    if (auto *f = dynamic_cast<const ForStmt *>(stmt)) {
        std::string loop = genLabel("forloop");
        std::string cont = genLabel("forcont");
        std::string end = genLabel("forend");
        emitStmt(f->getInit(), locals, endLabel);
        breakLabels.push_back(end);
        continueLabels.push_back(cont);
        loopFinallyDepth.push_back(finallyStack.size());
        out << loop << ":\n";
        if (f->getCondition()) {
            emitExpr(f->getCondition(), locals);
            out << "    cmp rax,0\n";
            out << "    je " << end << "\n";
        }
        emitStmt(f->getBody(), locals, endLabel);
        out << cont << ":\n";
        emitStmt(f->getPost(), locals, endLabel);
        out << "    jmp " << loop << "\n";
        out << end << ":\n";
        breakLabels.pop_back();
        continueLabels.pop_back();
        loopFinallyDepth.pop_back();
        return true;
    }
    if (auto *dw = dynamic_cast<const DoWhileStmt *>(stmt)) {
        std::string loop = genLabel("doloop");
        std::string cont = genLabel("docont");
        std::string end = genLabel("doend");
        breakLabels.push_back(end);
        continueLabels.push_back(cont);
        loopFinallyDepth.push_back(finallyStack.size());
        out << loop << ":\n";
        emitStmt(dw->getBody(), locals, endLabel);
        out << cont << ":\n";
        emitExpr(dw->getCondition(), locals);
        out << "    cmp rax,0\n";
        out << "    jne " << loop << "\n";
        out << end << ":\n";
        breakLabels.pop_back();
        continueLabels.pop_back();
        loopFinallyDepth.pop_back();
        return true;
    }
    if (auto *sw = dynamic_cast<const SwitchStmt *>(stmt)) {
        emitExpr(sw->getExpr(), locals);
        out << "    mov rbx, rax\n";
        bool switchIsString = isStringExpr(sw->getExpr(), locals);
        auto emitSwitchCompare = [&](const Expr *caseExpr, const std::string &label) {
            auto emitCompareOne = [&](const Expr *valueExpr) {
                if (auto *rangeCase = dynamic_cast<const CallExpr*>(valueExpr)) {
                    if (rangeCase->getName() == "__rango_case__" &&
                        rangeCase->getArgs().size() == 2) {
                        std::string rangeNoMatch = genLabel("case_rng_no");
                        emitExpr(rangeCase->getArgs()[0].get(), locals);
                        out << "    mov rcx, rax\n";
                        emitExpr(rangeCase->getArgs()[1].get(), locals);
                        out << "    mov rdx, rax\n";
                        out << "    cmp rbx, rcx\n";
                        out << "    jl " << rangeNoMatch << "\n";
                        out << "    cmp rbx, rdx\n";
                        out << "    jle " << label << "\n";
                        out << rangeNoMatch << ":\n";
                        return;
                    }
                }
                emitExpr(valueExpr, locals);
                if (switchIsString) {
                    out << "    mov " << reg1(this->windows) << ", rbx\n";
                    out << "    mov " << reg2(this->windows) << ", rax\n";
                    out << "    call strcmp\n";
                    out << "    cmp rax,0\n";
                    out << "    je " << label << "\n";
                } else {
                    out << "    cmp rbx, rax\n";
                    out << "    je " << label << "\n";
                }
            };
            if (auto *listCase = dynamic_cast<const ListExpr*>(caseExpr)) {
                for (const auto &option : listCase->getElements()) {
                    emitCompareOne(option.get());
                }
            } else {
                emitCompareOne(caseExpr);
            }
        };
        std::string end = genLabel("switchend");
        breakLabels.push_back(end);
        std::vector<std::string> labels;
        for (size_t i = 0; i < sw->getCases().size(); ++i)
            labels.push_back(genLabel("case"));
        std::string defLabel = sw->getDefault() ? genLabel("defcase") : end;
        size_t idx = 0;
        for (const auto &c : sw->getCases()) {
            emitSwitchCompare(c.first.get(), labels[idx]);
            ++idx;
        }
        if (sw->getDefault())
            out << "    jmp " << defLabel << "\n";
        else
            out << "    jmp " << end << "\n";
        idx = 0;
        for (const auto &c : sw->getCases()) {
            out << labels[idx] << ":\n";
            emitStmt(c.second.get(), locals, endLabel);
            ++idx;
        }
        if (sw->getDefault()) {
            out << defLabel << ":\n";
            emitStmt(sw->getDefault(), locals, endLabel);
        }
        out << end << ":\n";
        breakLabels.pop_back();
        return true;
    }
    if (dynamic_cast<const BreakStmt *>(stmt)) {
        size_t limit = loopFinallyDepth.empty() ? 0 : loopFinallyDepth.back();
        for (size_t i = finallyStack.size(); i > limit; --i) {
            out << "    call " << finallyStack[i - 1] << "\n";
        }
        if (!breakLabels.empty())
            out << "    jmp " << breakLabels.back() << "\n";
        return true;
    }
    if (dynamic_cast<const ContinueStmt *>(stmt)) {
        size_t limit = loopFinallyDepth.empty() ? 0 : loopFinallyDepth.back();
        for (size_t i = finallyStack.size(); i > limit; --i) {
            out << "    call " << finallyStack[i - 1] << "\n";
        }
        if (!continueLabels.empty())
            out << "    jmp " << continueLabels.back() << "\n";
        return true;
    }
    if (auto *ret = dynamic_cast<const ReturnStmt *>(stmt)) {
        if (ret->getValue()) emitExpr(ret->getValue(), locals);
        if (!finallyStack.empty()) {
            if (ret->getValue()) {
                int spillPad = this->windows ? 40 : 8;
                out << "    push rax\n";
                out << "    sub rsp, " << spillPad << "\n";
            }
            for (size_t i = finallyStack.size(); i > 0; --i) {
                out << "    call " << finallyStack[i - 1] << "\n";
            }
            if (ret->getValue()) {
                int spillPad = this->windows ? 40 : 8;
                out << "    add rsp, " << spillPad << "\n";
                out << "    pop rax\n";
            }
        }
        out << "    jmp " << endLabel << "\n";
        return true;
    }
    return false;
}

bool CodeGenImpl::emitStmtException(const Stmt *stmt,
                                   const std::unordered_map<std::string,int> *locals,
                                   const std::string &endLabel) {
    if (auto *thr = dynamic_cast<const ThrowStmt *>(stmt)) {
        if (thr->getMessage()) {
            emitExpr(thr->getMessage(), locals);
        } else {
            out << "    mov rax, 0\n";
        }
        int spillPad = this->windows ? 40 : 8;
        out << "    push rax\n";
        out << "    sub rsp, " << spillPad << "\n";
        if (thr->getType()) {
            emitExpr(thr->getType(), locals);
        } else {
            size_t idx = findString("Error");
            out << "    lea rax, [rel str" << idx << "]\n";
        }
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    add rsp, " << spillPad << "\n";
        out << "    pop " << reg2(this->windows) << "\n";
        out << "    call aym_exception_new\n";
        if (!throwFinallyLimitStack.empty() && throwFinallyLimitStack.back() != SIZE_MAX) {
            spillPad = this->windows ? 40 : 8;
            out << "    push rax\n";
            out << "    sub rsp, " << spillPad << "\n";
            size_t limit = throwFinallyLimitStack.back();
            for (size_t i = finallyStack.size(); i > limit; --i) {
                out << "    call " << finallyStack[i - 1] << "\n";
            }
            out << "    add rsp, " << spillPad << "\n";
            out << "    pop rax\n";
        }
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_throw\n";
        return true;
    }
    if (auto *t = dynamic_cast<const TryStmt *>(stmt)) {
        std::string catchLabel = genLabel("catch");
        std::string end = genLabel("tryend");
        std::string finallyLabel;
        if (t->getFinallyBlock()) finallyLabel = genLabel("finally");

        out << "    call aym_try_push\n";
        if (locals && locals->count(t->getHandlerSlot())) {
            out << "    mov [rbp-" << locals->at(t->getHandlerSlot()) << "], rax\n";
        } else {
            out << "    mov [rel " << t->getHandlerSlot() << "], rax\n";
        }
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call aym_try_env\n";
        out << "    mov " << reg1(this->windows) << ", rax\n";
        out << "    call setjmp\n";
        out << "    cmp rax,0\n";
        out << "    jne " << catchLabel << "\n";

        if (t->getFinallyBlock()) finallyStack.push_back(finallyLabel);
        emitStmt(t->getTryBlock(), locals, endLabel);
        if (t->getFinallyBlock()) finallyStack.pop_back();

        if (locals && locals->count(t->getHandlerSlot())) {
            out << "    mov " << reg1(this->windows) << ", [rbp-" << locals->at(t->getHandlerSlot()) << "]\n";
        } else {
            out << "    mov " << reg1(this->windows) << ", [rel " << t->getHandlerSlot() << "]\n";
        }
        out << "    call aym_try_pop\n";
        if (t->getFinallyBlock()) out << "    call " << finallyLabel << "\n";
        out << "    jmp " << end << "\n";

        out << catchLabel << ":\n";
        if (locals && locals->count(t->getHandlerSlot())) {
            out << "    mov " << reg1(this->windows) << ", [rbp-" << locals->at(t->getHandlerSlot()) << "]\n";
        } else {
            out << "    mov " << reg1(this->windows) << ", [rel " << t->getHandlerSlot() << "]\n";
        }
        out << "    call aym_try_get_exception\n";
        if (locals && locals->count(t->getExceptionSlot())) {
            out << "    mov [rbp-" << locals->at(t->getExceptionSlot()) << "], rax\n";
        } else {
            out << "    mov [rel " << t->getExceptionSlot() << "], rax\n";
        }
        if (locals && locals->count(t->getHandlerSlot())) {
            out << "    mov " << reg1(this->windows) << ", [rbp-" << locals->at(t->getHandlerSlot()) << "]\n";
        } else {
            out << "    mov " << reg1(this->windows) << ", [rel " << t->getHandlerSlot() << "]\n";
        }
        out << "    call aym_try_pop\n";

        if (t->getCatches().empty()) {
            if (t->getFinallyBlock()) out << "    call " << finallyLabel << "\n";
            if (locals && locals->count(t->getExceptionSlot())) {
                out << "    mov " << reg1(this->windows) << ", [rbp-" << locals->at(t->getExceptionSlot()) << "]\n";
            } else {
                out << "    mov " << reg1(this->windows) << ", [rel " << t->getExceptionSlot() << "]\n";
            }
            out << "    call aym_throw\n";
        } else {
            std::string noMatch = genLabel("catch_nomatch");
            for (size_t idx = 0; idx < t->getCatches().size(); ++idx) {
                const auto &c = t->getCatches()[idx];
                std::string nextLabel = genLabel("catch_next");
                if (!c.typeName.empty()) {
                    if (locals && locals->count(t->getExceptionSlot())) {
                        out << "    mov " << reg1(this->windows) << ", [rbp-" << locals->at(t->getExceptionSlot()) << "]\n";
                    } else {
                        out << "    mov " << reg1(this->windows) << ", [rel " << t->getExceptionSlot() << "]\n";
                    }
                    out << "    call aym_exception_type\n";
                    out << "    mov " << reg2(this->windows) << ", rax\n";
                    size_t typeIdx = findString(c.typeName);
                    out << "    lea " << reg1(this->windows) << ", [rel str" << typeIdx << "]\n";
                    out << "    call strcmp\n";
                    out << "    cmp rax,0\n";
                    out << "    jne " << nextLabel << "\n";
                }
                if (locals && locals->count(c.varName)) {
                    if (locals && locals->count(t->getExceptionSlot())) {
                        out << "    mov rax, [rbp-" << locals->at(t->getExceptionSlot()) << "]\n";
                    } else {
                        out << "    mov rax, [rel " << t->getExceptionSlot() << "]\n";
                    }
                    out << "    mov [rbp-" << locals->at(c.varName) << "], rax\n";
                } else {
                    if (locals && locals->count(t->getExceptionSlot())) {
                        out << "    mov rax, [rbp-" << locals->at(t->getExceptionSlot()) << "]\n";
                    } else {
                        out << "    mov rax, [rel " << t->getExceptionSlot() << "]\n";
                    }
                    out << "    mov [rel " << c.varName << "], rax\n";
                }
                if (t->getFinallyBlock()) finallyStack.push_back(finallyLabel);
                if (t->getFinallyBlock()) {
                    throwFinallyLimitStack.push_back(finallyStack.size() - 1);
                } else {
                    throwFinallyLimitStack.push_back(SIZE_MAX);
                }
                emitStmt(c.block.get(), locals, endLabel);
                throwFinallyLimitStack.pop_back();
                if (t->getFinallyBlock()) finallyStack.pop_back();
                if (t->getFinallyBlock()) out << "    call " << finallyLabel << "\n";
                out << "    jmp " << end << "\n";
                out << nextLabel << ":\n";
            }
            out << noMatch << ":\n";
            if (t->getFinallyBlock()) out << "    call " << finallyLabel << "\n";
            if (locals && locals->count(t->getExceptionSlot())) {
                out << "    mov " << reg1(this->windows) << ", [rbp-" << locals->at(t->getExceptionSlot()) << "]\n";
            } else {
                out << "    mov " << reg1(this->windows) << ", [rel " << t->getExceptionSlot() << "]\n";
            }
            out << "    call aym_throw\n";
        }

        if (t->getFinallyBlock()) {
            out << "    jmp " << end << "\n";
            out << finallyLabel << ":\n";
            int finallySpillPad = this->windows ? 40 : 8;
            out << "    sub rsp, " << finallySpillPad << "\n";
            emitStmt(t->getFinallyBlock(), locals, endLabel);
            out << "    add rsp, " << finallySpillPad << "\n";
            out << "    ret\n";
        }
        out << end << ":\n";
        return true;
    }
    return false;
}

} // namespace aym
