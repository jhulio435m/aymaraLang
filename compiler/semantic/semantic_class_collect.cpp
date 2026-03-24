#include "semantic.h"
#include "../utils/class_names.h"

#include <iostream>
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

} // namespace aym
