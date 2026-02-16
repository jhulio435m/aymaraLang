#include "semantic.h"

namespace aym {

void SemanticAnalyzer::visit(PrintStmt &p) {
    markNode(p);
    for (const auto &expr : p.getExprs()) {
        if (expr) expr->accept(*this);
    }
    if (p.getSeparator()) p.getSeparator()->accept(*this);
    if (p.getTerminator()) p.getTerminator()->accept(*this);
}

void SemanticAnalyzer::visit(ExprStmt &e) {
    markNode(e);
    if (e.getExpr()) e.getExpr()->accept(*this);
}

void SemanticAnalyzer::visit(AssignStmt &a) {
    markNode(a);
    a.getValue()->accept(*this);
    std::string t = currentType;
    if (lastInputCall) t = lookup(a.getName());
    if (!isDeclared(a.getName())) {
        declare(a.getName(), t);
    } else if (!t.empty() && !isTypeAssignable(t, lookup(a.getName()))) {
        reportError("tipo incompatible en asignacion a '" + a.getName() + "'", "AYM3003");
    }
}

void SemanticAnalyzer::visit(IndexAssignStmt &a) {
    markNode(a);
    a.getBase()->accept(*this);
    std::string baseType = currentType;
    a.getIndex()->accept(*this);
    a.getValue()->accept(*this);
    std::string valueType = currentType;
    if (baseType.rfind("kasta-ref:", 0) == 0) {
        std::string className = baseType.substr(10);
        auto *indexLiteral = dynamic_cast<StringExpr*>(a.getIndex());
        if (!indexLiteral) {
            reportError("acceso de atributo estatico invalido");
        } else {
            const FieldInfo *field = lookupStaticField(className, indexLiteral->getValue());
            if (field && field->isPrivate && currentClass != field->ownerClass) {
                reportError("atributo estatico privado no accesible");
            }
            if (field && !isTypeAssignable(valueType, field->type)) {
                reportError("tipo incompatible en asignacion estatica");
            }
        }
        currentType = valueType;
    } else if (baseType.rfind("kasta:", 0) == 0) {
        std::string className = baseType.substr(6);
        auto *indexLiteral = dynamic_cast<StringExpr*>(a.getIndex());
        if (!indexLiteral) {
            reportError("acceso de atributo invalido");
        } else {
            const FieldInfo *field = lookupField(className, indexLiteral->getValue());
            if (field && field->isPrivate && currentClass != field->ownerClass) {
                reportError("atributo privado no accesible");
            }
            if (field && !isTypeAssignable(valueType, field->type)) {
                reportError("tipo incompatible en asignacion de atributo");
            }
        }
        currentType = valueType;
    } else if (baseType.rfind("t'aqa:", 0) == 0) {
        std::string elementType = baseType.substr(6);
        if (!elementType.empty() && valueType != elementType) {
            reportError("tipo incompatible en asignacion de lista");
        }
        currentType = elementType;
    } else if (baseType.rfind("mapa:", 0) == 0) {
        std::string elementType = baseType.substr(5);
        if (!elementType.empty() && valueType != elementType) {
            reportError("tipo incompatible en asignacion de mapa");
        }
        currentType = elementType;
    } else if (baseType == "mapa") {
        currentType = valueType;
    } else {
        reportError("se esperaba una lista para asignacion por indice");
    }
}

void SemanticAnalyzer::visit(VarDeclStmt &v) {
    markNode(v);
    std::string t = "";
    if (v.getInit()) {
        v.getInit()->accept(*this);
        t = currentType;
        if (lastInputCall) t = v.getType();
    }
    std::string declaredType = v.getType();
    if (isClassName(declaredType)) {
        declaredType = "kasta:" + declaredType;
    }
    if (declaredType == "t'aqa") {
        if (t.rfind("t'aqa:", 0) == 0) {
            declaredType = t;
        } else if (t.empty()) {
            declaredType = "t'aqa:jakhüwi";
        }
    }
    if (declaredType == "mapa") {
        if (t.rfind("mapa:", 0) == 0) {
            declaredType = t;
        } else if (t.empty()) {
            declaredType = "mapa:jakhüwi";
        }
    }
    declare(v.getName(), declaredType);
    if (!t.empty() && !isTypeAssignable(t, declaredType) && !isTypeAssignable(t, v.getType())) {
        reportError("tipo incompatible en declaracion de '" + v.getName() + "'", "AYM3003");
    }
}

void SemanticAnalyzer::visit(BlockStmt &b) {
    markNode(b);
    pushScope();
    for (const auto &s : b.statements) s->accept(*this);
    popScope();
}

void SemanticAnalyzer::visit(IfStmt &i) {
    markNode(i);
    i.getCondition()->accept(*this);
    i.getThen()->accept(*this);
    if (i.getElse()) i.getElse()->accept(*this);
}

void SemanticAnalyzer::visit(WhileStmt &w) {
    markNode(w);
    if (w.getCondition()) w.getCondition()->accept(*this);
    ++loopDepth;
    w.getBody()->accept(*this);
    --loopDepth;
}

void SemanticAnalyzer::visit(DoWhileStmt &dw) {
    markNode(dw);
    ++loopDepth;
    dw.getBody()->accept(*this);
    --loopDepth;
    dw.getCondition()->accept(*this);
}

void SemanticAnalyzer::visit(ForStmt &f) {
    markNode(f);
    pushScope();
    f.getInit()->accept(*this);
    if (f.getCondition()) f.getCondition()->accept(*this);
    f.getPost()->accept(*this);
    ++loopDepth;
    f.getBody()->accept(*this);
    --loopDepth;
    popScope();
}

void SemanticAnalyzer::visit(SwitchStmt &sw) {
    markNode(sw);
    sw.getExpr()->accept(*this);
    std::string switchType = currentType;
    ++switchDepth;
    for (const auto &c : sw.getCases()) {
        auto checkCaseType = [&](Expr *expr) {
            if (auto *call = dynamic_cast<CallExpr*>(expr)) {
                if (call->getName() == "__rango_case__") {
                    if (switchType != "jakhüwi") {
                        reportError("los rangos en match solo aplican a 'jakhüwi'");
                    }
                    if (call->getArgs().size() != 2) {
                        reportError("rango invalido en case");
                        return;
                    }
                    call->getArgs()[0]->accept(*this);
                    std::string startType = currentType;
                    call->getArgs()[1]->accept(*this);
                    std::string endType = currentType;
                    if (startType != "jakhüwi" || endType != "jakhüwi") {
                        reportError("los extremos del rango deben ser 'jakhüwi'");
                    }
                    currentType = "jakhüwi";
                    return;
                }
            }
            expr->accept(*this);
            std::string caseType = currentType;
            if (!switchType.empty() && !caseType.empty() && switchType != caseType) {
                reportError("tipo incompatible en match: se esperaba '" + switchType + "' y se obtuvo '" + caseType + "'");
            }
        };
        if (auto *listCase = dynamic_cast<ListExpr*>(c.first.get())) {
            if (listCase->getElements().empty()) {
                reportError("case vacio en match");
            }
            for (const auto &option : listCase->getElements()) {
                checkCaseType(option.get());
            }
        } else {
            checkCaseType(c.first.get());
        }
        c.second->accept(*this);
    }
    if (sw.getDefault()) sw.getDefault()->accept(*this);
    --switchDepth;
}

void SemanticAnalyzer::visit(ImportStmt &imp) {
    markNode(imp);
    // Los modulos se resuelven antes del analisis semantico.
}

void SemanticAnalyzer::visit(FunctionStmt &fn) {
    markNode(fn);
    pushScope();
    ++functionDepth;
    auto it = paramTypes.find(fn.getName());
    size_t idx = 0;
    for (const auto &param : fn.getParams()) {
        std::string t = "jakhüwi";
        if (it != paramTypes.end() && idx < it->second.size()) t = it->second[idx];
        if (t == "t'aqa") t = "t'aqa:jakhüwi";
        declare(param.name, t);
        ++idx;
    }
    fn.getBody()->accept(*this);
    --functionDepth;
    popScope();
}

void SemanticAnalyzer::visit(BreakStmt &brk) {
    markNode(brk);
    if (loopDepth == 0 && switchDepth == 0) {
        reportError("'break' fuera de un ciclo o switch", "AYM3006");
    }
}

void SemanticAnalyzer::visit(ContinueStmt &cont) {
    markNode(cont);
    if (loopDepth == 0) {
        reportError("'continue' fuera de un ciclo", "AYM3007");
    }
}

void SemanticAnalyzer::visit(ReturnStmt &r) {
    markNode(r);
    if (functionDepth == 0) {
        reportError("'return' fuera de una funcion", "AYM3008");
    }
    if (r.getValue()) r.getValue()->accept(*this);
}


void SemanticAnalyzer::visit(ThrowStmt &t) {
    markNode(t);
    if (t.getType()) t.getType()->accept(*this);
    if (t.getMessage()) t.getMessage()->accept(*this);
    currentType = "";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(TryStmt &t) {
    markNode(t);
    t.getTryBlock()->accept(*this);
    for (const auto &c : t.getCatches()) {
        pushScope();
        declare(c.varName, "excepcion");
        c.block->accept(*this);
        popScope();
    }
    if (t.getFinallyBlock()) t.getFinallyBlock()->accept(*this);
}

} // namespace aym
