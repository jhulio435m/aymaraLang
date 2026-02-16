#include "semantic.h"
#include "../utils/class_names.h"

namespace aym {

void SemanticAnalyzer::visit(FunctionRefExpr &f) {
    markNode(f);
    currentType = "jakhüwi";
    lastInputCall = false;
}

void SemanticAnalyzer::visit(ListExpr &l) {
    markNode(l);
    std::string elementType;
    for (const auto &elem : l.getElements()) {
        elem->accept(*this);
        std::string t = currentType;
        if (elementType.empty()) {
            elementType = t;
        } else if (elementType != t) {
            reportError("tipos incompatibles en lista");
        }
    }
    if (elementType.empty()) {
        elementType = "jakhüwi";
    }
    currentType = "t'aqa:" + elementType;
    lastInputCall = false;
}

void SemanticAnalyzer::visit(MapExpr &m) {
    markNode(m);
    std::string valueType;
    bool sawValue = false;
    for (const auto &item : m.getItems()) {
        item.first->accept(*this);
        std::string keyType = currentType;
        if (keyType != "aru") {
            reportError("clave de mapa debe ser texto");
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
    markNode(i);
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
        reportError("se esperaba una lista para indexacion");
        currentType = "";
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(MemberExpr &m) {
    markNode(m);
    m.getBase()->accept(*this);
    std::string baseType = currentType;
    if (baseType == "excepcion") {
        m.setExceptionAccess(true);
        currentType = "aru";
        m.setResolvedType("aru");
    } else if (baseType.rfind("kasta:", 0) == 0) {
        std::string className = baseType.substr(6);
        const FieldInfo *field = lookupField(className, m.getMember());
        if (!field) {
            reportError("atributo '" + m.getMember() + "' no existe en '" + className + "'");
            currentType = "";
        } else if (field->isPrivate && currentClass != field->ownerClass) {
            reportError("atributo privado '" + m.getMember() + "' no accesible");
            currentType = "";
        } else {
            currentType = field->type;
            m.setResolvedType(field->type);
        }
    } else if (baseType.rfind("kasta-ref:", 0) == 0) {
        std::string className = baseType.substr(10);
        const FieldInfo *field = lookupStaticField(className, m.getMember());
        if (!field) {
            reportError("atributo estatico '" + m.getMember() + "' no existe en '" + className + "'");
            currentType = "";
        } else if (field->isPrivate && currentClass != field->ownerClass) {
            reportError("atributo estatico privado '" + m.getMember() + "' no accesible");
            currentType = "";
        } else {
            m.setStaticField(classStaticFieldName(className, m.getMember()));
            currentType = field->type;
            m.setResolvedType(field->type);
        }
    } else if (baseType.rfind("mapa:", 0) == 0) {
        std::string valueType = baseType.substr(5);
        if (valueType.empty()) valueType = "jakhüwi";
        currentType = valueType;
        m.setResolvedType(valueType);
    } else if (baseType == "mapa") {
        currentType = "jakhüwi";
        m.setResolvedType("jakhüwi");
    } else {
        reportError("acceso de miembro invalido");
        currentType = "";
    }
    lastInputCall = false;
}


} // namespace aym
