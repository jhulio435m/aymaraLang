#include "codegen_impl.h"

namespace aym {

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

} // namespace aym

