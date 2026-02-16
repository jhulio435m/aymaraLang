#include "semantic.h"
#include "../utils/class_names.h"

#include <algorithm>

namespace aym {

void SemanticAnalyzer::visit(ClassStmt &cls) {
    markNode(cls);
    std::string prevClass = currentClass;
    std::string prevBase = currentBaseClass;
    currentClass = cls.getName();
    currentBaseClass = cls.getBase();

    for (const auto &field : cls.getFields()) {
        if (field.init) {
            field.init->accept(*this);
        }
    }

    for (const auto &method : cls.getMethods()) {
        pushScope();
        ++functionDepth;
        if (!method.isStatic) {
            declare("Aka", "kasta:" + cls.getName());
        }
        size_t idx = 0;
        std::string fnName = method.isStatic
            ? classStaticMethodName(cls.getName(), method.name)
            : classMethodName(cls.getName(), method.name);
        auto it = paramTypes.find(fnName);
        for (const auto &param : method.params) {
            std::string t = param.type;
            if (t == "t'aqa") t = "t'aqa:jakhüwi";
            if (t == "mapa") t = "mapa:jakhüwi";
            if (isClassName(t)) t = "kasta:" + t;
            if (it != paramTypes.end()) {
                size_t offset = method.isStatic ? idx : idx + 1;
                if (offset < it->second.size()) {
                    t = it->second[offset];
                }
            }
            declare(param.name, t);
            ++idx;
        }
        if (method.body) method.body->accept(*this);
        --functionDepth;
        popScope();
    }

    for (const auto &ctor : cls.getConstructors()) {
        pushScope();
        ++functionDepth;
        declare("Aka", "kasta:" + cls.getName());
        size_t idx = 0;
        std::string fnName = classCtorName(cls.getName(), ctor.params.size());
        auto it = paramTypes.find(fnName);
        for (const auto &param : ctor.params) {
            std::string t = param.type;
            if (t == "t'aqa") t = "t'aqa:jakhüwi";
            if (t == "mapa") t = "mapa:jakhüwi";
            if (isClassName(t)) t = "kasta:" + t;
            if (it != paramTypes.end()) {
                size_t offset = idx + 1;
                if (offset < it->second.size()) {
                    t = it->second[offset];
                }
            }
            declare(param.name, t);
            ++idx;
        }
        if (ctor.body) ctor.body->accept(*this);
        --functionDepth;
        popScope();
    }

    currentClass = prevClass;
    currentBaseClass = prevBase;
}

void SemanticAnalyzer::visit(MemberCallExpr &c) {
    markNode(c);
    c.getBase()->accept(*this);
    std::string baseType = currentType;
    auto validateArgs = [&](const MethodInfo *mi) {
        size_t argc = c.getArgs().size();
        if (argc != mi->paramTypes.size()) {
            reportError("numero incorrecto de argumentos en '" + c.getMember() + "'", "AYM3005");
        }
        size_t count = std::min(argc, mi->paramTypes.size());
        for (size_t i = 0; i < count; ++i) {
            c.getArgs()[i]->accept(*this);
            if (!isTypeAssignable(currentType, mi->paramTypes[i])) {
                reportError("tipo incompatible en argumento " + std::to_string(i + 1) +
                            " de '" + c.getMember() + "'");
            }
        }
        for (size_t i = count; i < argc; ++i) {
            c.getArgs()[i]->accept(*this);
        }
    };
    if (dynamic_cast<SuperExpr*>(c.getBase())) {
        if (currentBaseClass.empty()) {
            reportError("'jilaaka' fuera de clase");
            currentType = "";
        } else {
            const MethodInfo *mi = lookupMethod(currentBaseClass, c.getMember());
            if (!mi) {
                reportError("metodo '" + c.getMember() + "' no existe en '" + currentBaseClass + "'");
                currentType = "";
            } else if (mi->isPrivate) {
                reportError("metodo privado '" + c.getMember() + "' no accesible desde jilaaka");
                currentType = "";
            } else {
                validateArgs(mi);
                currentType = mi->returnType;
            }
        }
    } else if (baseType.rfind("kasta-ref:", 0) == 0) {
        std::string className = baseType.substr(10);
        const MethodInfo *mi = lookupStaticMethod(className, c.getMember());
        if (!mi) {
            reportError("metodo estatico '" + c.getMember() + "' no existe en '" + className + "'");
            currentType = "";
        } else if (mi->isPrivate && currentClass != mi->ownerClass) {
            reportError("metodo estatico privado '" + c.getMember() + "' no accesible");
            currentType = "";
        } else {
            validateArgs(mi);
            c.setStaticCallee(classStaticMethodName(className, c.getMember()));
            currentType = mi->returnType;
        }
    } else if (baseType.rfind("kasta:", 0) == 0) {
        std::string className = baseType.substr(6);
        const MethodInfo *mi = lookupMethod(className, c.getMember());
        if (!mi) {
            reportError("metodo '" + c.getMember() + "' no existe en '" + className + "'");
            currentType = "";
        } else if (mi->isPrivate && currentClass != mi->ownerClass) {
            reportError("metodo privado '" + c.getMember() + "' no accesible");
            currentType = "";
        } else {
            validateArgs(mi);
            currentType = mi->returnType;
        }
    } else {
        reportError("llamada de metodo invalida");
        currentType = "";
    }
    c.setResolvedType(currentType);
    lastInputCall = false;
}

void SemanticAnalyzer::visit(NewExpr &n) {
    markNode(n);
    if (!isClassName(n.getName())) {
        reportError("clase '" + n.getName() + "' no definida", "AYM3010");
        currentType = "";
        return;
    }
    const ClassInfo *info = lookupClass(n.getName());
    bool ctorOk = false;
    if (info) {
        size_t arity = n.getArgs().size();
        if (info->constructors.empty() && arity != 0) {
            reportError("no hay constructor con " + std::to_string(arity) +
                        " parametros en '" + n.getName() + "'");
        } else if (!info->constructors.empty() && info->constructors.find(arity) == info->constructors.end()) {
            reportError("no hay constructor con " + std::to_string(arity) +
                        " parametros en '" + n.getName() + "'");
        } else {
            auto it = info->constructors.find(arity);
            if (it != info->constructors.end()) {
                bool typesOk = true;
                for (size_t i = 0; i < arity; ++i) {
                    n.getArgs()[i]->accept(*this);
                    if (!isTypeAssignable(currentType, it->second[i])) {
                        reportError("tipo incompatible en argumento " + std::to_string(i + 1) +
                                    " de constructor '" + n.getName() + "'");
                        typesOk = false;
                    }
                }
                ctorOk = typesOk;
            } else if (arity == 0) {
                ctorOk = true;
            }
        }
    }

    if (!ctorOk) {
        for (const auto &arg : n.getArgs()) {
            arg->accept(*this);
        }
        currentType = "";
    } else {
        currentType = "kasta:" + n.getName();
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(SuperExpr &s) {
    markNode(s);
    if (currentClass.empty() || currentBaseClass.empty()) {
        reportError("'jilaaka' fuera de clase");
        currentType = "";
    } else {
        currentType = "kasta:" + currentBaseClass;
    }
    lastInputCall = false;
}


} // namespace aym
