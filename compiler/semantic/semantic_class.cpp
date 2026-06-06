#include "semantic.h"
#include "../utils/class_names.h"
#include <iostream>
#include <algorithm>
#include <unordered_set>

namespace aym {

void SemanticAnalyzer::collectClassInfo(const std::vector<std::unique_ptr<Node>> &nodes) {
    std::unordered_set<std::string> classNames;
    for (const auto &n : nodes) {
        auto *cls = dynamic_cast<const ClassStmt*>(n.get());
        if (cls) {
            markNode(*cls);
            if (!classNames.insert(cls->getName()).second) {
                reportError("clase '" + cls->getName() + "' declarada mas de una vez");
            }
        }
    }

    for (const auto &n : nodes) {
        auto *cls = dynamic_cast<const ClassStmt*>(n.get());
        if (!cls) continue;
        markNode(*cls);
        ClassInfo info;
        info.name = cls->getName();
        info.base = cls->getBase();
        if (!info.base.empty() && !classNames.count(info.base)) {
            reportError("clase base '" + info.base + "' no definida", "AYM3010");
        }
        for (const auto &field : cls->getFields()) {
            std::string type = field.type;
            if (type == "t'aqa") type = "t'aqa:jakhüwi";
            if (type == "mapa") type = "mapa:jakhüwi";
            if (classNames.count(type)) type = "kasta:" + type;
            FieldInfo fi;
            fi.type = type;
            fi.isStatic = field.isStatic;
            fi.isPrivate = field.isPrivate;
            fi.ownerClass = cls->getName();
            if (field.isStatic) {
                if (info.staticFields.count(field.name) || info.fields.count(field.name)) {
                    reportError("atributo '" + field.name + "' duplicado en clase '" + cls->getName() + "'");
                }
                info.staticFields[field.name] = fi;
            } else {
                if (info.fields.count(field.name) || info.staticFields.count(field.name)) {
                    reportError("atributo '" + field.name + "' duplicado en clase '" + cls->getName() + "'");
                }
                info.fields[field.name] = fi;
            }
        }
        for (const auto &method : cls->getMethods()) {
            MethodInfo mi;
            mi.returnType = method.returnType;
            if (mi.returnType.empty()) mi.returnType = "";
            if (mi.returnType == "t'aqa") mi.returnType = "t'aqa:jakhüwi";
            if (mi.returnType == "mapa") mi.returnType = "mapa:jakhüwi";
            if (classNames.count(mi.returnType)) mi.returnType = "kasta:" + mi.returnType;
            for (const auto &p : method.params) {
                std::string t = p.type;
                if (t == "t'aqa") t = "t'aqa:jakhüwi";
                if (t == "mapa") t = "mapa:jakhüwi";
                if (classNames.count(t)) t = "kasta:" + t;
                mi.paramTypes.push_back(t);
            }
            mi.isStatic = method.isStatic;
            mi.isPrivate = method.isPrivate;
            mi.ownerClass = cls->getName();
            if (method.isStatic) {
                if (info.staticMethods.count(method.name) || info.methods.count(method.name)) {
                    reportError("metodo '" + method.name + "' duplicado en clase '" + cls->getName() + "'");
                }
                info.staticMethods[method.name] = mi;
            } else {
                if (info.methods.count(method.name) || info.staticMethods.count(method.name)) {
                    reportError("metodo '" + method.name + "' duplicado en clase '" + cls->getName() + "'");
                }
                info.methods[method.name] = mi;
            }
            std::string fnName = method.isStatic
                ? classStaticMethodName(cls->getName(), method.name)
                : classMethodName(cls->getName(), method.name);
            std::vector<std::string> fnParamTypes;
            if (!method.isStatic) {
                fnParamTypes.push_back("kasta:" + cls->getName());
            }
            fnParamTypes.insert(fnParamTypes.end(), mi.paramTypes.begin(), mi.paramTypes.end());
            paramTypes[fnName] = fnParamTypes;
            functions[fnName] = fnParamTypes.size();
            if (!mi.returnType.empty()) {
                functionReturnTypes[fnName] = mi.returnType;
            }
        }
        for (const auto &ctor : cls->getConstructors()) {
            std::vector<std::string> ctorTypes;
            for (const auto &p : ctor.params) {
                std::string t = p.type;
                if (t == "t'aqa") t = "t'aqa:jakhüwi";
                if (t == "mapa") t = "mapa:jakhüwi";
                if (classNames.count(t)) t = "kasta:" + t;
                ctorTypes.push_back(t);
            }
            if (info.constructors.count(ctor.params.size())) {
                reportError("constructor duplicado con " + std::to_string(ctor.params.size()) +
                            " parametros en '" + cls->getName() + "'");
            }
            info.constructors[ctor.params.size()] = ctorTypes;
            std::string ctorName = classCtorName(cls->getName(), ctor.params.size());
            std::vector<std::string> ctorParamTypes;
            ctorParamTypes.push_back("kasta:" + cls->getName());
            ctorParamTypes.insert(ctorParamTypes.end(), ctorTypes.begin(), ctorTypes.end());
            paramTypes[ctorName] = ctorParamTypes;
            functions[ctorName] = ctorParamTypes.size();
        }
        classes[cls->getName()] = std::move(info);
    }

    // Validate inheritance cycles.
    for (const auto &entry : classes) {
        std::unordered_set<std::string> seen;
        const ClassInfo *info = &entry.second;
        while (info && !info->base.empty()) {
            if (!seen.insert(info->name).second) {
                reportError("ciclo de herencia detectado en '" + entry.first + "'");
                break;
            }
            info = lookupClass(info->base);
        }
    }

    // Validate inherited method compatibility. Redefining a base method is an implicit override.
    for (const auto &n : nodes) {
        auto *cls = dynamic_cast<const ClassStmt*>(n.get());
        if (!cls) continue;
        markNode(*cls);
        if (cls->getBase().empty()) continue;
        const ClassInfo *own = lookupClass(cls->getName());
        if (!own) continue;
        for (const auto &method : cls->getMethods()) {
            const MethodInfo *baseInfo = lookupMethod(cls->getBase(), method.name);
            if (!baseInfo) continue;
            if (method.isStatic) {
                reportError("metodo estatico '" + method.name + "' no puede redefinir un metodo heredado", "AYM3009");
                continue;
            }

            auto ownIt = own->methods.find(method.name);
            if (ownIt == own->methods.end()) continue;
            const MethodInfo &ownInfo = ownIt->second;

            if (baseInfo->isPrivate) {
                reportError("metodo '" + method.name + "' no puede redefinir un metodo privado heredado", "AYM3009");
                continue;
            }
            if (ownInfo.paramTypes != baseInfo->paramTypes) {
                reportError("firma incompatible al redefinir metodo heredado '" + method.name + "'", "AYM3009");
            }
            if (ownInfo.returnType != baseInfo->returnType) {
                reportError("tipo de retorno incompatible al redefinir metodo heredado '" + method.name + "'", "AYM3009");
            }
        }
    }

    for (const auto &pair : classes) {
        const auto &info = pair.second;
        for (const auto &field : info.staticFields) {
            declare(classStaticFieldName(info.name, field.first), field.second.type);
        }
    }
}

bool SemanticAnalyzer::isClassName(const std::string &name) const {
    return classes.find(name) != classes.end();
}

const SemanticAnalyzer::ClassInfo *SemanticAnalyzer::lookupClass(const std::string &name) const {
    auto it = classes.find(name);
    if (it == classes.end()) return nullptr;
    return &it->second;
}

const SemanticAnalyzer::MethodInfo *SemanticAnalyzer::lookupMethod(const std::string &className,
                                                                    const std::string &methodName) const {
    const ClassInfo *info = lookupClass(className);
    if (!info) return nullptr;
    auto it = info->methods.find(methodName);
    if (it != info->methods.end()) return &it->second;
    if (!info->base.empty()) {
        return lookupMethod(info->base, methodName);
    }
    return nullptr;
}

const SemanticAnalyzer::MethodInfo *SemanticAnalyzer::lookupStaticMethod(const std::string &className,
                                                                          const std::string &methodName) const {
    const ClassInfo *info = lookupClass(className);
    if (!info) return nullptr;
    auto it = info->staticMethods.find(methodName);
    if (it != info->staticMethods.end()) return &it->second;
    if (!info->base.empty()) {
        return lookupStaticMethod(info->base, methodName);
    }
    return nullptr;
}

const SemanticAnalyzer::FieldInfo *SemanticAnalyzer::lookupField(const std::string &className,
                                                                 const std::string &fieldName) const {
    const ClassInfo *info = lookupClass(className);
    if (!info) return nullptr;
    auto it = info->fields.find(fieldName);
    if (it != info->fields.end()) return &it->second;
    if (!info->base.empty()) {
        return lookupField(info->base, fieldName);
    }
    return nullptr;
}

const SemanticAnalyzer::FieldInfo *SemanticAnalyzer::lookupStaticField(const std::string &className,
                                                                       const std::string &fieldName) const {
    const ClassInfo *info = lookupClass(className);
    if (!info) return nullptr;
    auto it = info->staticFields.find(fieldName);
    if (it != info->staticFields.end()) return &it->second;
    if (!info->base.empty()) {
        return lookupStaticField(info->base, fieldName);
    }
    return nullptr;
}

bool SemanticAnalyzer::isSubclassOf(const std::string &className, const std::string &baseName) const {
    if (className == baseName) return true;
    const ClassInfo *info = lookupClass(className);
    std::unordered_set<std::string> seen;
    while (info && !info->base.empty()) {
        if (!seen.insert(info->name).second) return false;
        if (info->base == baseName) return true;
        info = lookupClass(info->base);
    }
    return false;
}

bool SemanticAnalyzer::isTypeAssignable(const std::string &actualType,
                                        const std::string &expectedType) const {
    if (expectedType.empty() || actualType.empty()) return false;
    if (actualType == expectedType) return true;
    if (actualType.rfind("kasta:", 0) == 0 && expectedType.rfind("kasta:", 0) == 0) {
        return isSubclassOf(actualType.substr(6), expectedType.substr(6));
    }
    return false;
}

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
