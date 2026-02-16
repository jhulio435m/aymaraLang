#include "semantic.h"

namespace aym {

void SemanticAnalyzer::visit(NumberExpr &n) {
    markNode(n);
    currentType = "jakhüwi";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(BoolExpr &b) {
    markNode(b);
    currentType = "chiqa";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(StringExpr &s) {
    markNode(s);
    currentType = "aru";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(VariableExpr &v) {
    markNode(v);
    if (!isDeclared(v.getName())) {
        if (isClassName(v.getName())) {
            currentType = "kasta-ref:" + v.getName();
        } else {
            reportError("variable '" + v.getName() + "' no declarada", "AYM3002");
            currentType = "";
        }
    } else {
        currentType = lookup(v.getName());
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(BinaryExpr &b) {
    markNode(b);
    b.getLeft()->accept(*this);
    std::string l = currentType;
    b.getRight()->accept(*this);
    std::string r = currentType;
    if (l != r) {
        reportError("tipos incompatibles en operacion", "AYM3003");
    }
    char op = b.getOp();
    if ((l == "aru" || r == "aru") && op != '+' && op != 's' && op != 'd') {
        reportError("operacion invalida sobre textos", "AYM3003");
    }
    if (op=='&' || op=='|' || op=='s' || op=='d' || op=='<' || op=='>' || op=='l' || op=='g')
        currentType = "chiqa";
    else
        currentType = l;
    lastInputCall = false;
}

void SemanticAnalyzer::visit(UnaryExpr &u) {
    markNode(u);
    u.getExpr()->accept(*this);
    if (u.getOp() == '!') {
        currentType = "chiqa";
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(TernaryExpr &t) {
    markNode(t);
    t.getCondition()->accept(*this);
    std::string condType = currentType;
    if (condType != "chiqa") {
        reportError("condicion del ternario debe ser booleana", "AYM3003");
    }
    t.getThen()->accept(*this);
    std::string thenType = currentType;
    t.getElse()->accept(*this);
    std::string elseType = currentType;
    if (thenType != elseType) {
        reportError("tipos incompatibles en operador ternario", "AYM3003");
    }
    currentType = thenType;
    lastInputCall = false;
}

void SemanticAnalyzer::visit(IncDecExpr &e) {
    markNode(e);
    if (!isDeclared(e.getName())) {
        reportError("variable '" + e.getName() + "' no declarada", "AYM3002");
        currentType = "";
    } else {
        std::string t = lookup(e.getName());
        if (t != "jakhüwi") {
            reportError("incremento/decremento requiere numero", "AYM3003");
        }
        currentType = t;
    }
    lastInputCall = false;
}

} // namespace aym
