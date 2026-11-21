#include "semantic.h"
#include "../builtins/builtins.h"
#include "../utils/class_names.h"
#include "../utils/diagnostic.h"
#include <iostream>
#include <algorithm>

namespace aym {

void SemanticAnalyzer::markNode(const Node &node) {
    currentLine = node.getLine();
    currentColumn = node.getColumn();
}

void SemanticAnalyzer::reportError(const std::string &message, const std::string &code) {
    hadErrors = true;
    if (diagnostics) {
        diagnostics->error(code, message, currentLine, currentColumn);
        return;
    }
    if (currentLine > 0) {
        std::cerr << "Error en linea " << currentLine << ", columna " << currentColumn
                  << ": " << message << std::endl;
        return;
    }
    std::cerr << "Error: " << message << std::endl;
}

void SemanticAnalyzer::pushScope() {
    scopes.emplace_back();
    constScopes.emplace_back();
}

void SemanticAnalyzer::popScope() {
    if (!scopes.empty()) scopes.pop_back();
    if (!constScopes.empty()) constScopes.pop_back();
}

void SemanticAnalyzer::declare(const std::string &name, const std::string &type) {
    if (scopes.empty()) pushScope();
    scopes.back()[name] = type;
}

bool SemanticAnalyzer::isDeclared(const std::string &name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->count(name)) return true;
    }
    return false;
}

bool SemanticAnalyzer::isConstVar(const std::string &name) const {
    for (size_t i = scopes.size(); i > 0; --i) {
        size_t idx = i - 1;
        if (scopes[idx].count(name)) {
            return idx < constScopes.size() && constScopes[idx].count(name) > 0;
        }
    }
    return false;
}

std::string SemanticAnalyzer::lookup(const std::string &name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return f->second;
    }
    return "";
}

std::string SemanticAnalyzer::resolveTypeAnnotation(const std::string &type) const {
    if (type.rfind("t'aqa:", 0) == 0) {
        return "t'aqa:" + resolveTypeAnnotation(type.substr(6));
    }
    if (type.rfind("mapa:", 0) == 0) {
        return "mapa:" + resolveTypeAnnotation(type.substr(5));
    }
    if (isClassName(type)) {
        return "kasta:" + type;
    }
    return type;
}

std::unique_ptr<Expr> SemanticAnalyzer::cloneDefaultExpr(const Expr *def) {
    if (!def) return nullptr;
    if (auto *n = dynamic_cast<const NumberExpr *>(def)) {
        auto node = std::make_unique<NumberExpr>(n->getValue());
        node->setLocation(n->getLine(), n->getColumn());
        return node;
    }
    if (auto *f = dynamic_cast<const FloatExpr *>(def)) {
        auto node = std::make_unique<FloatExpr>(f->getValue());
        node->setLocation(f->getLine(), f->getColumn());
        return node;
    }
    if (auto *b = dynamic_cast<const BoolExpr *>(def)) {
        auto node = std::make_unique<BoolExpr>(b->getValue());
        node->setLocation(b->getLine(), b->getColumn());
        return node;
    }
    if (auto *s = dynamic_cast<const StringExpr *>(def)) {
        auto node = std::make_unique<StringExpr>(s->getValue());
        node->setLocation(s->getLine(), s->getColumn());
        return node;
    }
    if (auto *u = dynamic_cast<const UnaryExpr *>(def)) {
        auto inner = cloneDefaultExpr(u->getExpr());
        if (!inner) return nullptr;
        auto node = std::make_unique<UnaryExpr>(u->getOp(), std::move(inner));
        node->setLocation(u->getLine(), u->getColumn());
        return node;
    }
    if (auto *bin = dynamic_cast<const BinaryExpr *>(def)) {
        auto lhs = cloneDefaultExpr(bin->getLeft());
        auto rhs = cloneDefaultExpr(bin->getRight());
        if (!lhs || !rhs) return nullptr;
        auto node = std::make_unique<BinaryExpr>(bin->getOp(), std::move(lhs), std::move(rhs));
        node->setLocation(bin->getLine(), bin->getColumn());
        return node;
    }
    if (auto *list = dynamic_cast<const ListExpr *>(def)) {
        std::vector<std::unique_ptr<Expr>> elems;
        elems.reserve(list->getElements().size());
        for (const auto &elem : list->getElements()) {
            auto cloned = cloneDefaultExpr(elem.get());
            if (!cloned) return nullptr;
            elems.push_back(std::move(cloned));
        }
        auto node = std::make_unique<ListExpr>(std::move(elems));
        node->setLocation(list->getLine(), list->getColumn());
        return node;
    }
    return nullptr;
}

void SemanticAnalyzer::analyze(const std::vector<std::unique_ptr<Node>> &nodes) {
    hadErrors = false;
    scopes.clear();
    constScopes.clear();
    globals.clear();
    globalTypes.clear();
    functions.clear();
    paramTypes.clear();
    paramDefaults.clear();
    methodDefaults.clear();
    functionReturnTypes.clear();
    classes.clear();
    currentClass.clear();
    currentBaseClass.clear();
    loopDepth = 0;
    switchDepth = 0;
    functionDepth = 0;
    currentType.clear();
    lastInputCall = false;
    currentLine = 0;
    currentColumn = 0;

    pushScope();
    for (const auto &p : getBuiltinFunctions()) {
        functions[p.first] = p.second.argCount;
        if (!p.second.paramTypes.empty()) {
            std::vector<std::string> types;
            for (auto t : p.second.paramTypes) types.push_back(typeName(t));
            paramTypes[p.first] = types;
        }
    }
    collectClassInfo(nodes);

    struct FuncCollector : ASTVisitor {
        SemanticAnalyzer *self;
        void visit(FunctionStmt &fn) override {
            self->functions[fn.getName()] = fn.getParams().size();
            {
                std::vector<std::shared_ptr<Expr>> defaults;
                bool anyDefault = false;
                for (const auto &p : fn.getParams()) {
                    defaults.push_back(p.defaultValue);
                    if (p.defaultValue) anyDefault = true;
                }
                if (anyDefault) self->paramDefaults[fn.getName()] = std::move(defaults);
            }
            std::vector<std::string> types;
            for (const auto &p : fn.getParams()) {
                types.push_back(self->resolveTypeAnnotation(p.type));
            }
            self->paramTypes[fn.getName()] = std::move(types);
            if (!fn.getReturnType().empty()) {
                self->functionReturnTypes[fn.getName()] = self->resolveTypeAnnotation(fn.getReturnType());
            }
        }
        void visit(NumberExpr&) override {}
        void visit(FloatExpr&) override {}
        void visit(BoolExpr&) override {}
        void visit(StringExpr&) override {}
        void visit(VariableExpr&) override {}
        void visit(BinaryExpr&) override {}
        void visit(UnaryExpr&) override {}
        void visit(TernaryExpr&) override {}
        void visit(IncDecExpr&) override {}
        void visit(CallExpr&) override {}
        void visit(ListExpr&) override {}
        void visit(MapExpr&) override {}
        void visit(IndexExpr&) override {}
        void visit(MemberExpr&) override {}
        void visit(PrintStmt&) override {}
        void visit(ExprStmt&) override {}
        void visit(AssignStmt&) override {}
        void visit(IndexAssignStmt&) override {}
        void visit(BlockStmt&) override {}
        void visit(IfStmt&) override {}
        void visit(ForStmt&) override {}
        void visit(BreakStmt&) override {}
        void visit(ContinueStmt&) override {}
        void visit(ReturnStmt&) override {}
        void visit(VarDeclStmt&) override {}
        void visit(WhileStmt&) override {}
        void visit(DoWhileStmt&) override {}
        void visit(SwitchStmt&) override {}
        void visit(ImportStmt&) override {}
        void visit(ThrowStmt&) override {}
        void visit(TryStmt&) override {}
        void visit(ClassStmt&) override {}
        void visit(MemberCallExpr&) override {}
        void visit(NewExpr&) override {}
        void visit(FunctionRefExpr&) override {}
        void visit(SuperExpr&) override {}
    } collector;
    collector.self = this;
    for (const auto &n : nodes) n->accept(collector);

    for (const auto &n : nodes) n->accept(*this);
    globals.clear();
    globalTypes.clear();
    if (!scopes.empty()) {
        for (const auto &p : scopes.front()) {
            globals.insert(p.first);
            globalTypes[p.first] = p.second;
        }
    }
    popScope();
}

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
    if (isConstVar(a.getName())) {
        reportError("no se puede modificar la constante '" + a.getName() + "'", "AYM3003");
    }
    a.getValue()->accept(*this);
    std::string t = currentType;
    if (lastInputCall) t = lookup(a.getName());
    if (!isDeclared(a.getName())) {
        declare(a.getName(), t);
    } else if (!t.empty() && !checkAssignable(a.getValue(), t, lookup(a.getName()))) {
        std::string msg = "tipo incompatible en asignacion a '" + a.getName() + "'";
        if (t == "chikata" && lookup(a.getName()) == "jakhüwi") {
            msg += " (chikata no se convierte solo a jakhüwi: usa jakhüwi(x), floor(x) o round(x))";
        }
        reportError(msg, "AYM3003");
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
            if (field && !checkAssignable(a.getValue(), valueType, field->type)) {
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
            if (field && !checkAssignable(a.getValue(), valueType, field->type)) {
                reportError("tipo incompatible en asignacion de atributo");
            }
        }
        currentType = valueType;
    } else if (baseType.rfind("t'aqa:", 0) == 0) {
        std::string elementType = baseType.substr(6);
        if (!valueType.empty() && !elementType.empty() && !checkAssignable(a.getValue(), valueType, elementType)) {
            reportError("tipo incompatible en asignacion de lista");
        }
        currentType = elementType;
    } else if (baseType == "t'aqa") {
        currentType = valueType;
    } else if (baseType.rfind("mapa:", 0) == 0) {
        std::string elementType = baseType.substr(5);
        if (!valueType.empty() && !elementType.empty() && !checkAssignable(a.getValue(), valueType, elementType)) {
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
    std::string declaredType = resolveTypeAnnotation(v.getType());
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
    if (v.isConstDecl()) {
        if (!v.getInit()) {
            reportError("constante requiere inicializacion", "AYM3001");
        }
        if (!constScopes.empty()) {
            constScopes.back().insert(v.getName());
        }
    } else if (!constScopes.empty()) {
        constScopes.back().erase(v.getName());
    }
    if (!t.empty() && !checkAssignable(v.getInit(), t, declaredType) && !isTypeAssignable(t, v.getType())) {
        std::string msg = "tipo incompatible en declaracion de '" + v.getName() + "'";
        if (t == "chikata" && declaredType == "jakhüwi") {
            msg += " (chikata no se convierte solo a jakhüwi: usa jakhüwi(x), floor(x) o round(x))";
        }
        reportError(msg, "AYM3003");
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
    markCondition(i.getCondition());
    i.getThen()->accept(*this);
    if (i.getElse()) i.getElse()->accept(*this);
}

void SemanticAnalyzer::visit(WhileStmt &w) {
    markNode(w);
    if (w.getCondition()) {
        w.getCondition()->accept(*this);
        markCondition(w.getCondition());
    }
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
    markCondition(dw.getCondition());
}

void SemanticAnalyzer::visit(ForStmt &f) {
    markNode(f);
    pushScope();
    f.getInit()->accept(*this);
    if (f.getCondition()) {
        f.getCondition()->accept(*this);
        markCondition(f.getCondition());
    }
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
                    if (!isNumericType(switchType)) {
                        reportError("los rangos en 'khiti' solo aplican a 'jakhüwi' o 'chikata'");
                    }
                    if (call->getArgs().size() != 2) {
                        reportError("rango invalido en 'kuna'");
                        return;
                    }
                    call->getArgs()[0]->accept(*this);
                    std::string startType = currentType;
                    call->getArgs()[1]->accept(*this);
                    std::string endType = currentType;
                    if (switchType == "chikata" && isNumericType(startType) && isNumericType(endType)) {
                        markToFloat(call->getArgs()[0].get());
                        markToFloat(call->getArgs()[1].get());
                    } else if (startType != "jakhüwi" || endType != "jakhüwi") {
                        reportError("los extremos del rango deben ser 'jakhüwi'");
                    }
                    currentType = switchType;
                    return;
                }
            }
            expr->accept(*this);
            std::string caseType = currentType;
            if (switchType == "chikata" && caseType == "jakhüwi") {
                markToFloat(expr);
                caseType = "chikata";
            }
            if (!switchType.empty() && !caseType.empty() && switchType != caseType) {
                reportError("tipo incompatible en 'khiti': se esperaba '" + switchType + "' y se obtuvo '" + caseType + "'");
            }
        };
        if (auto *listCase = dynamic_cast<ListExpr*>(c.first.get())) {
            if (listCase->getElements().empty()) {
                reportError("'kuna' vacio en 'khiti'");
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
        std::string t = resolveTypeAnnotation(param.type);
        if (it != paramTypes.end() && idx < it->second.size()) t = it->second[idx];
        declare(param.name, t);
        ++idx;
    }
    std::string prevReturn = currentReturnType;
    currentReturnType = resolveTypeAnnotation(fn.getReturnType());
    fn.getBody()->accept(*this);
    currentReturnType = prevReturn;
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
    if (r.getValue()) {
        r.getValue()->accept(*this);
        std::string t = currentType;
        if (!currentReturnType.empty() && !t.empty() &&
            !checkAssignable(r.getValue(), t, currentReturnType)) {
            reportError("tipo incompatible en 'kuttaya': se esperaba '" + currentReturnType +
                        "' y se obtuvo '" + t + "'", "AYM3003");
        }
    }
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
