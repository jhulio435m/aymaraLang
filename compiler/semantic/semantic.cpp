#include "semantic.h"
#include "../builtins/builtins.h"
#include <iostream>
#include <algorithm>

namespace aym {

void SemanticAnalyzer::pushScope() {
    scopes.emplace_back();
}

void SemanticAnalyzer::popScope() {
    if (!scopes.empty()) scopes.pop_back();
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

std::string SemanticAnalyzer::lookup(const std::string &name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return f->second;
    }
    return "";
}

void SemanticAnalyzer::analyze(const std::vector<std::unique_ptr<Node>> &nodes) {
    pushScope();
    for (const auto &p : getBuiltinFunctions()) {
        functions[p.first] = p.second.argCount;
        if (!p.second.paramTypes.empty()) {
            std::vector<std::string> types;
            for (auto t : p.second.paramTypes) types.push_back(typeName(t));
            paramTypes[p.first] = types;
        }
    }

    struct FuncCollector : ASTVisitor {
        SemanticAnalyzer *self;
        void visit(FunctionStmt &fn) override {
            self->functions[fn.getName()] = fn.getParams().size();
            std::vector<std::string> types;
            for (const auto &p : fn.getParams()) {
                types.push_back(p.type);
            }
            self->paramTypes[fn.getName()] = std::move(types);
            if (!fn.getReturnType().empty()) {
                self->functionReturnTypes[fn.getName()] = fn.getReturnType();
            }
        }
        void visit(NumberExpr&) override {}
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
    for (const auto &expr : p.getExprs()) {
        if (expr) expr->accept(*this);
    }
    if (p.getSeparator()) p.getSeparator()->accept(*this);
    if (p.getTerminator()) p.getTerminator()->accept(*this);
}

void SemanticAnalyzer::visit(ExprStmt &e) {
    if (e.getExpr()) e.getExpr()->accept(*this);
}

void SemanticAnalyzer::visit(AssignStmt &a) {
    a.getValue()->accept(*this);
    std::string t = currentType;
    if (lastInputCall) t = lookup(a.getName());
    if (!isDeclared(a.getName())) {
        declare(a.getName(), t);
    } else if (lookup(a.getName()) != t && !t.empty()) {
        std::cerr << "Error: tipo incompatible en asignacion a '" << a.getName() << "'" << std::endl;
    }
}

void SemanticAnalyzer::visit(IndexAssignStmt &a) {
    a.getBase()->accept(*this);
    std::string baseType = currentType;
    a.getIndex()->accept(*this);
    a.getValue()->accept(*this);
    std::string valueType = currentType;
    if (baseType.rfind("t'aqa:", 0) == 0) {
        std::string elementType = baseType.substr(6);
        if (!elementType.empty() && valueType != elementType) {
            std::cerr << "Error: tipo incompatible en asignacion de lista" << std::endl;
        }
        currentType = elementType;
    } else if (baseType.rfind("mapa:", 0) == 0) {
        std::string elementType = baseType.substr(5);
        if (!elementType.empty() && valueType != elementType) {
            std::cerr << "Error: tipo incompatible en asignacion de mapa" << std::endl;
        }
        currentType = elementType;
    } else if (baseType == "mapa") {
        currentType = valueType;
    } else {
        std::cerr << "Error: se esperaba una lista para asignacion por indice" << std::endl;
    }
}

void SemanticAnalyzer::visit(VarDeclStmt &v) {
    std::string t = "";
    if (v.getInit()) {
        v.getInit()->accept(*this);
        t = currentType;
        if (lastInputCall) t = v.getType();
    }
    std::string declaredType = v.getType();
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
    if (!t.empty() && t != declaredType && t != v.getType()) {
        std::cerr << "Error: tipo incompatible en declaracion de '" << v.getName() << "'" << std::endl;
    }
}

void SemanticAnalyzer::visit(BlockStmt &b) {
    pushScope();
    for (const auto &s : b.statements) s->accept(*this);
    popScope();
}

void SemanticAnalyzer::visit(IfStmt &i) {
    i.getCondition()->accept(*this);
    i.getThen()->accept(*this);
    if (i.getElse()) i.getElse()->accept(*this);
}

void SemanticAnalyzer::visit(WhileStmt &w) {
    if (w.getCondition()) w.getCondition()->accept(*this);
    ++loopDepth;
    w.getBody()->accept(*this);
    --loopDepth;
}

void SemanticAnalyzer::visit(DoWhileStmt &dw) {
    ++loopDepth;
    dw.getBody()->accept(*this);
    --loopDepth;
    dw.getCondition()->accept(*this);
}

void SemanticAnalyzer::visit(ForStmt &f) {
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
    sw.getExpr()->accept(*this);
    ++switchDepth;
    for (const auto &c : sw.getCases()) {
        c.first->accept(*this);
        c.second->accept(*this);
    }
    if (sw.getDefault()) sw.getDefault()->accept(*this);
    --switchDepth;
}

void SemanticAnalyzer::visit(ImportStmt &) {
    // Los modulos se resuelven antes del analisis semantico.
}

void SemanticAnalyzer::visit(FunctionStmt &fn) {
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

void SemanticAnalyzer::visit(BreakStmt &) {
    if (loopDepth == 0 && switchDepth == 0) {
        std::cerr << "Error: 'break' fuera de un ciclo o switch" << std::endl;
    }
}

void SemanticAnalyzer::visit(ContinueStmt &) {
    if (loopDepth == 0) {
        std::cerr << "Error: 'continue' fuera de un ciclo" << std::endl;
    }
}

void SemanticAnalyzer::visit(ReturnStmt &r) {
    if (functionDepth == 0) {
        std::cerr << "Error: 'return' fuera de una funcion" << std::endl;
    }
    if (r.getValue()) r.getValue()->accept(*this);
}

void SemanticAnalyzer::visit(NumberExpr &) {
    currentType = "jakhüwi";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(BoolExpr &) {
    currentType = "chiqa";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(StringExpr &) {
    currentType = "aru";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(VariableExpr &v) {
    if (!isDeclared(v.getName())) {
        std::cerr << "Error: variable '" << v.getName() << "' no declarada" << std::endl;
        currentType = "";
    } else {
        currentType = lookup(v.getName());
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(BinaryExpr &b) {
    b.getLeft()->accept(*this);
    std::string l = currentType;
    b.getRight()->accept(*this);
    std::string r = currentType;
    if (l != r) {
        std::cerr << "Error: tipos incompatibles en operacion" << std::endl;
    }
    char op = b.getOp();
    if ((l == "aru" || r == "aru") && op != '+' && op != 's' && op != 'd') {
        std::cerr << "Error: operacion invalida sobre textos" << std::endl;
    }
    if (op=='&' || op=='|' || op=='s' || op=='d' || op=='<' || op=='>' || op=='l' || op=='g')
        currentType = "chiqa";
    else
        currentType = l;
    lastInputCall = false;
}

void SemanticAnalyzer::visit(UnaryExpr &u) {
    u.getExpr()->accept(*this);
    if (u.getOp() == '!') {
        currentType = "chiqa";
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(TernaryExpr &t) {
    t.getCondition()->accept(*this);
    std::string condType = currentType;
    if (condType != "chiqa") {
        std::cerr << "Error: condicion del ternario debe ser booleana" << std::endl;
    }
    t.getThen()->accept(*this);
    std::string thenType = currentType;
    t.getElse()->accept(*this);
    std::string elseType = currentType;
    if (thenType != elseType) {
        std::cerr << "Error: tipos incompatibles en operador ternario" << std::endl;
    }
    currentType = thenType;
    lastInputCall = false;
}

void SemanticAnalyzer::visit(IncDecExpr &e) {
    if (!isDeclared(e.getName())) {
        std::cerr << "Error: variable '" << e.getName() << "' no declarada" << std::endl;
        currentType = "";
    } else {
        std::string t = lookup(e.getName());
        if (t != "jakhüwi") {
            std::cerr << "Error: incremento/decremento requiere numero" << std::endl;
        }
        currentType = t;
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(CallExpr &c) {
    std::string nameLower = c.getName();
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    auto it = functions.find(c.getName());
    if (it == functions.end()) {
        it = functions.find(nameLower);
    }
    if (it == functions.end()) {
        std::cerr << "Error: funcion '" << c.getName() << "' no declarada" << std::endl;
    } else if (nameLower == "katu") {
        if (c.getArgs().size() < 1 || c.getArgs().size() > 2) {
            std::cerr << "Error: numero incorrecto de argumentos en llamada a '" << c.getName() << "'" << std::endl;
        }
    } else if (nameLower == BUILTIN_CHANI_M) {
        if (c.getArgs().size() < 2 || c.getArgs().size() > 3) {
            std::cerr << "Error: numero incorrecto de argumentos en llamada a '" << c.getName() << "'" << std::endl;
        }
    } else if (c.getArgs().size() != it->second) {
        std::cerr << "Error: numero incorrecto de argumentos en llamada a '" << c.getName() << "'" << std::endl;
    }
    size_t idx = 0;
    auto pit = paramTypes.find(c.getName());
    if (pit == paramTypes.end()) {
        pit = paramTypes.find(nameLower);
    }
    for (const auto &arg : c.getArgs()) {
        arg->accept(*this);
        std::string t = currentType;
        if (pit != paramTypes.end() && idx < pit->second.size()) {
            if (t == "aru") pit->second[idx] = "aru";
        }
        ++idx;
    }
    if (nameLower == BUILTIN_TO_STRING || nameLower == BUILTIN_CHUSA ||
        nameLower == BUILTIN_MAYACHTA || nameLower == BUILTIN_SIKTA) {
        currentType = "aru";
    } else if (nameLower == BUILTIN_TO_NUMBER) {
        currentType = "jakhüwi";
    } else if (nameLower == "katu") {
        currentType = "aru";
    } else if (nameLower == "largo" || nameLower == BUILTIN_SUYU || nameLower == BUILTIN_SUYUT) {
        currentType = "jakhüwi";
    } else if (nameLower == BUILTIN_SUYU_M) {
        currentType = "jakhüwi";
    } else if (nameLower == BUILTIN_UTJI || nameLower == BUILTIN_UTJIT || nameLower == BUILTIN_UTJI_SUTI) {
        currentType = "chiqa";
    } else if (nameLower == BUILTIN_JALJTA) {
        currentType = "t'aqa:aru";
    } else if (nameLower == BUILTIN_SUTINAKA) {
        currentType = "t'aqa:aru";
    } else if (nameLower == BUILTIN_CHANINAKA) {
        if (!c.getArgs().empty()) {
            c.getArgs()[0]->accept(*this);
            std::string baseType = currentType;
            if (baseType.rfind("mapa:", 0) == 0) {
                currentType = "t'aqa:" + baseType.substr(5);
            } else {
                currentType = "t'aqa:jakhüwi";
            }
        } else {
            currentType = "t'aqa:jakhüwi";
        }
    } else if (nameLower == BUILTIN_CHANI_M) {
        if (c.getArgs().size() == 3) {
            c.getArgs()[2]->accept(*this);
            currentType = currentType;
        } else if (!c.getArgs().empty()) {
            c.getArgs()[0]->accept(*this);
            std::string baseType = currentType;
            if (baseType.rfind("mapa:", 0) == 0) {
                currentType = baseType.substr(5);
            } else {
                currentType = "jakhüwi";
            }
        } else {
            currentType = "jakhüwi";
        }
    } else if (nameLower == "push" || nameLower == BUILTIN_CHULLU) {
        if (!c.getArgs().empty()) {
            c.getArgs()[0]->accept(*this);
            std::string baseType = currentType;
            if (c.getArgs().size() > 1) {
                c.getArgs()[1]->accept(*this);
                std::string valueType = currentType;
                if (baseType.rfind("t'aqa:", 0) == 0) {
                    std::string elementType = baseType.substr(6);
                    if (!elementType.empty() && valueType != elementType) {
                        std::cerr << "Error: tipo incompatible en push" << std::endl;
                    }
                    currentType = baseType;
                } else {
                    std::cerr << "Error: se esperaba una lista para push" << std::endl;
                }
            } else {
                currentType = baseType;
            }
        } else {
            currentType = "t'aqa:jakhüwi";
        }
    } else if (nameLower == BUILTIN_APSU || nameLower == BUILTIN_APSU_UKA) {
        if (!c.getArgs().empty()) {
            c.getArgs()[0]->accept(*this);
            std::string baseType = currentType;
            if (baseType.rfind("t'aqa:", 0) == 0) {
                currentType = baseType.substr(6);
            } else {
                std::cerr << "Error: se esperaba una lista para " << c.getName() << std::endl;
                currentType = "";
            }
        } else {
            currentType = "";
        }
    } else {
        currentType = "jakhüwi";
    }
    lastInputCall = (nameLower == BUILTIN_INPUT);
}

void SemanticAnalyzer::visit(ListExpr &l) {
    std::string elementType;
    for (const auto &elem : l.getElements()) {
        elem->accept(*this);
        std::string t = currentType;
        if (elementType.empty()) {
            elementType = t;
        } else if (elementType != t) {
            std::cerr << "Error: tipos incompatibles en lista" << std::endl;
        }
    }
    if (elementType.empty()) {
        elementType = "jakhüwi";
    }
    currentType = "t'aqa:" + elementType;
    lastInputCall = false;
}

void SemanticAnalyzer::visit(MapExpr &m) {
    std::string valueType;
    bool sawValue = false;
    for (const auto &item : m.getItems()) {
        item.first->accept(*this);
        std::string keyType = currentType;
        if (keyType != "aru") {
            std::cerr << "Error: clave de mapa debe ser texto" << std::endl;
        }
        item.second->accept(*this);
        std::string t = currentType;
        if (!sawValue) {
            valueType = t;
            sawValue = true;
        } else if (!valueType.empty() && t != valueType) {
            valueType.clear();
        }
    }
    if (!sawValue) {
        currentType = "mapa:jakhüwi";
    } else if (!valueType.empty()) {
        currentType = "mapa:" + valueType;
    } else {
        currentType = "mapa";
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(IndexExpr &i) {
    i.getBase()->accept(*this);
    std::string baseType = currentType;
    i.getIndex()->accept(*this);
    if (baseType.rfind("t'aqa:", 0) == 0) {
        currentType = baseType.substr(6);
    } else if (baseType.rfind("mapa", 0) == 0) {
        if (baseType.rfind("mapa:", 0) == 0) {
            currentType = baseType.substr(5);
        } else {
            currentType = "jakhüwi";
        }
    } else {
        std::cerr << "Error: se esperaba una lista para indexacion" << std::endl;
        currentType = "";
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(MemberExpr &m) {
    m.getBase()->accept(*this);
    if (currentType != "excepcion") {
        std::cerr << "Error: acceso de miembro invalido" << std::endl;
    }
    currentType = "aru";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(ThrowStmt &t) {
    if (t.getType()) t.getType()->accept(*this);
    if (t.getMessage()) t.getMessage()->accept(*this);
    currentType = "";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(TryStmt &t) {
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
