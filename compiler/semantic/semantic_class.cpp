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
            classes[cls->getName()].name = cls->getName();
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
            std::string type = resolveTypeAnnotation(field.type);
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
            mi.returnType = resolveTypeAnnotation(method.returnType);
            std::vector<std::shared_ptr<Expr>> defaults;
            bool anyDefault = false;
            for (const auto &p : method.params) {
                mi.paramTypes.push_back(resolveTypeAnnotation(p.type));
                defaults.push_back(p.defaultValue);
                if (p.defaultValue) anyDefault = true;
            }
            if (anyDefault) {
                methodDefaults[cls->getName() + "::" + method.name] = std::move(defaults);
            }
            mi.isStatic = method.isStatic;
            mi.isPrivate = method.isPrivate;
            mi.isAbstract = method.isAbstract;
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
                ctorTypes.push_back(resolveTypeAnnotation(p.type));
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

    // Compute abstract classes (any unimplemented abstract method in hierarchy).
    for (auto &entry : classes) {
        std::unordered_map<std::string, bool> effectiveAbstract;
        std::unordered_set<std::string> visited;
        const ClassInfo *cur = &entry.second;
        while (cur && visited.insert(cur->name).second) {
            for (const auto &m : cur->methods) {
                if (effectiveAbstract.find(m.first) == effectiveAbstract.end()) {
                    effectiveAbstract[m.first] = m.second.isAbstract;
                }
            }
            if (cur->base.empty()) break;
            cur = lookupClass(cur->base);
        }
        bool hasUnimplemented = false;
        for (const auto &m : effectiveAbstract) {
            if (m.second) {
                hasUnimplemented = true;
                break;
            }
        }
        entry.second.isAbstract = hasUnimplemented;
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
    if ((actualType == "t'aqa" && expectedType.rfind("t'aqa:", 0) == 0) ||
        (expectedType == "t'aqa" && actualType.rfind("t'aqa:", 0) == 0)) {
        return true;
    }
    if ((actualType == "mapa" && expectedType.rfind("mapa:", 0) == 0) ||
        (expectedType == "mapa" && actualType.rfind("mapa:", 0) == 0)) {
        return true;
    }
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
            std::string t = resolveTypeAnnotation(param.type);
            if (it != paramTypes.end()) {
                size_t offset = method.isStatic ? idx : idx + 1;
                if (offset < it->second.size()) {
                    t = it->second[offset];
                }
            }
            declare(param.name, t);
            ++idx;
        }
        bool prevInCtor = inConstructor;
        inConstructor = false;
        if (method.body) method.body->accept(*this);
        inConstructor = prevInCtor;
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
            std::string t = resolveTypeAnnotation(param.type);
            if (it != paramTypes.end()) {
                size_t offset = idx + 1;
                if (offset < it->second.size()) {
                    t = it->second[offset];
                }
            }
            declare(param.name, t);
            ++idx;
        }
        bool prevInCtor = inConstructor;
        inConstructor = true;
        if (ctor.body) ctor.body->accept(*this);
        inConstructor = prevInCtor;
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
        if (c.getArgs().size() < mi->paramTypes.size()) {
            auto dit = methodDefaults.find(mi->ownerClass + "::" + c.getMember());
            if (dit != methodDefaults.end()) {
                fillDefaultArgs(c, dit->second);
            }
        }
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
    if (dynamic_cast<SuperExpr*>(c.getBase()) && c.getMember() == "qallta") {
        // jilaaka.qallta(args): constructor de la clase padre sobre 'aka'.
        const ClassInfo *baseInfo = currentBaseClass.empty() ? nullptr : lookupClass(currentBaseClass);
        size_t arity = c.getArgs().size();
        if (!baseInfo) {
            reportError("'jilaaka' fuera de clase");
        } else if (!inConstructor) {
            reportError("'jilaaka.qallta(...)' solo puede usarse dentro de un constructor");
        } else {
            // El padre puede heredar a su vez el constructor de un ancestro.
            const ClassInfo *owner = baseInfo;
            while (owner && owner->constructors.empty() && !owner->base.empty()) {
                owner = lookupClass(owner->base);
            }
            auto it = owner ? owner->constructors.find(arity) : decltype(baseInfo->constructors.end()){};
            if (owner && it != owner->constructors.end()) {
                for (size_t i = 0; i < arity; ++i) {
                    c.getArgs()[i]->accept(*this);
                    if (!isTypeAssignable(currentType, it->second[i])) {
                        reportError("tipo incompatible en argumento " + std::to_string(i + 1) +
                                    " de constructor '" + owner->name + "'");
                    }
                }
                c.setStaticCallee(classCtorName(owner->name, arity));
            } else if (arity == 0 && (!owner || owner->constructors.empty())) {
                // Padre sin constructor: no hay nada que invocar.
            } else {
                reportError("no hay constructor con " + std::to_string(arity) +
                            " parametros en '" + currentBaseClass + "'");
            }
        }
        currentType = "";
        c.setResolvedType(currentType);
        lastInputCall = false;
        return;
    }
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
            } else if (mi->isAbstract) {
                reportError("no se puede invocar metodo abstracto '" + c.getMember() + "' mediante jilaaka", "AYM3009");
                currentType = "";
            } else {
                validateArgs(mi);
                // Resolucion estatica: la clase padre es la del codigo donde
                // aparece 'jilaaka', no la del objeto en ejecucion.
                c.setStaticCallee(classMethodName(mi->ownerClass, c.getMember()));
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
            c.setStaticCallee(classStaticMethodName(mi->ownerClass, c.getMember()));
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
    if (info && info->isAbstract) {
        reportError("no se puede instanciar la clase abstracta '" + n.getName() + "'", "AYM3009");
        for (const auto &arg : n.getArgs()) {
            arg->accept(*this);
        }
        currentType = "";
        lastInputCall = false;
        return;
    }
    bool ctorOk = false;
    // Una clase sin constructores propios hereda los del ancestro mas cercano
    // que los defina.
    while (info && info->constructors.empty() && !info->base.empty()) {
        const ClassInfo *parent = lookupClass(info->base);
        if (!parent) break;
        info = parent;
    }
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
