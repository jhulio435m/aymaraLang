#include "codegen_impl.h"

namespace aym {

// El codegen no deduce tipos por su cuenta: usa el tipo que el analisis
// semantico anoto en cada expresion (Expr::getSemType). Asi hay una sola
// fuente de verdad para los tipos del lenguaje.
//
// Tipos genericos: una 't'aqa' o 'mapa' sin tipo de elemento (p. ej. un
// parametro 'lurawi f(t'aqa xs)') se trata como de elementos 'jakhüwi'.

namespace {

const std::string &semType(const Expr *e) {
    static const std::string empty;
    return e ? e->getSemType() : empty;
}

bool startsWith(const std::string &s, const char *prefix) {
    return s.rfind(prefix, 0) == 0;
}

} // namespace

bool CodeGenImpl::isStringExpr(const Expr *expr,
                               const std::unordered_map<std::string,int> *) const {
    return semType(expr) == "aru";
}

bool CodeGenImpl::isListExpr(const Expr *expr,
                             const std::unordered_map<std::string,int> *) const {
    return startsWith(semType(expr), "t'aqa");
}

bool CodeGenImpl::isMapExpr(const Expr *expr,
                            const std::unordered_map<std::string,int> *) const {
    const std::string &t = semType(expr);
    return startsWith(t, "mapa") || startsWith(t, "kasta:");
}

bool CodeGenImpl::isBoolExpr(const Expr *expr,
                             const std::unordered_map<std::string,int> *) const {
    return semType(expr) == "chiqa";
}

std::string CodeGenImpl::listElementType(const Expr *expr,
                                         const std::unordered_map<std::string,int> *) const {
    const std::string &t = semType(expr);
    if (startsWith(t, "t'aqa:")) return t.substr(6);
    if (t == "t'aqa") return "jakhüwi";
    return "";
}

std::string CodeGenImpl::mapValueType(const Expr *expr,
                                      const std::unordered_map<std::string,int> *) const {
    const std::string &t = semType(expr);
    if (startsWith(t, "mapa:")) return t.substr(5);
    if (t == "mapa") return "jakhüwi";
    return "";
}

} // namespace aym
