#include "semantic.h"
#include "../utils/class_names.h"

#include <iostream>
#include <unordered_set>

namespace aym {

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

std::string SemanticAnalyzer::lookupFieldType(const std::string &className,
                                              const std::string &fieldName) const {
    const ClassInfo *info = lookupClass(className);
    if (!info) return "";
    auto it = info->fields.find(fieldName);
    if (it != info->fields.end()) return it->second;
    if (!info->base.empty()) {
        return lookupFieldType(info->base, fieldName);
    }
    return "";
}

std::string SemanticAnalyzer::lookupStaticFieldType(const std::string &className,
                                                    const std::string &fieldName) const {
    const ClassInfo *info = lookupClass(className);
    if (!info) return "";
    auto it = info->staticFields.find(fieldName);
    if (it != info->staticFields.end()) return it->second;
    if (!info->base.empty()) {
        return lookupStaticFieldType(info->base, fieldName);
    }
    return "";
}

void SemanticAnalyzer::collectClassInfo(const std::vector<std::unique_ptr<Node>> &nodes) {
    std::unordered_set<std::string> classNames;
    for (const auto &n : nodes) {
        auto *cls = dynamic_cast<const ClassStmt*>(n.get());
        if (cls) {
            classNames.insert(cls->getName());
        }
    }

    for (const auto &n : nodes) {
        auto *cls = dynamic_cast<const ClassStmt*>(n.get());
        if (!cls) continue;
        ClassInfo info;
        info.name = cls->getName();
        info.base = cls->getBase();
        if (!info.base.empty() && !classNames.count(info.base)) {
            std::cerr << "Error: clase base '" << info.base << "' no definida" << std::endl;
        }
        for (const auto &field : cls->getFields()) {
            std::string type = field.type;
            if (type == "t'aqa") type = "t'aqa:jakhüwi";
            if (type == "mapa") type = "mapa:jakhüwi";
            if (classNames.count(type)) type = "kasta:" + type;
            if (field.isStatic) {
                info.staticFields[field.name] = type;
            } else {
                info.fields[field.name] = type;
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
            if (method.isStatic) {
                info.staticMethods[method.name] = mi;
            } else {
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

    for (const auto &pair : classes) {
        const auto &info = pair.second;
        for (const auto &field : info.staticFields) {
            declare(classStaticFieldName(info.name, field.first), field.second);
        }
    }
}

void SemanticAnalyzer::visit(ClassStmt &cls) {
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
    c.getBase()->accept(*this);
    std::string baseType = currentType;
    if (dynamic_cast<SuperExpr*>(c.getBase())) {
        if (currentBaseClass.empty()) {
            std::cerr << "Error: 'jilaaka' fuera de clase" << std::endl;
            currentType = "";
        } else {
            const MethodInfo *mi = lookupMethod(currentBaseClass, c.getMember());
            if (!mi) {
                std::cerr << "Error: metodo '" << c.getMember() << "' no existe en '" << currentBaseClass << "'" << std::endl;
                currentType = "";
            } else {
                if (c.getArgs().size() != mi->paramTypes.size()) {
                    std::cerr << "Error: numero incorrecto de argumentos en '" << c.getMember() << "'" << std::endl;
                }
                currentType = mi->returnType;
            }
        }
    } else if (baseType.rfind("kasta-ref:", 0) == 0) {
        std::string className = baseType.substr(10);
        const MethodInfo *mi = lookupStaticMethod(className, c.getMember());
        if (!mi) {
            std::cerr << "Error: metodo estatico '" << c.getMember() << "' no existe en '" << className << "'" << std::endl;
            currentType = "";
        } else {
            if (c.getArgs().size() != mi->paramTypes.size()) {
                std::cerr << "Error: numero incorrecto de argumentos en '" << c.getMember() << "'" << std::endl;
            }
            c.setStaticCallee(classStaticMethodName(className, c.getMember()));
            currentType = mi->returnType;
        }
    } else if (baseType.rfind("kasta:", 0) == 0) {
        std::string className = baseType.substr(6);
        const MethodInfo *mi = lookupMethod(className, c.getMember());
        if (!mi) {
            std::cerr << "Error: metodo '" << c.getMember() << "' no existe en '" << className << "'" << std::endl;
            currentType = "";
        } else {
            if (c.getArgs().size() != mi->paramTypes.size()) {
                std::cerr << "Error: numero incorrecto de argumentos en '" << c.getMember() << "'" << std::endl;
            }
            currentType = mi->returnType;
        }
    } else {
        std::cerr << "Error: llamada de metodo invalida" << std::endl;
        currentType = "";
    }
    c.setResolvedType(currentType);
    for (const auto &arg : c.getArgs()) {
        arg->accept(*this);
    }
    lastInputCall = false;
}

void SemanticAnalyzer::visit(NewExpr &n) {
    if (!isClassName(n.getName())) {
        std::cerr << "Error: clase '" << n.getName() << "' no definida" << std::endl;
        currentType = "";
        return;
    }
    const ClassInfo *info = lookupClass(n.getName());
    if (info) {
        size_t arity = n.getArgs().size();
        if (!info->constructors.empty() && info->constructors.find(arity) == info->constructors.end()) {
            std::cerr << "Error: no hay constructor con " << arity << " parametros en '" << n.getName() << "'" << std::endl;
        }
    }
    for (const auto &arg : n.getArgs()) {
        arg->accept(*this);
    }
    currentType = "kasta:" + n.getName();
    lastInputCall = false;
}

void SemanticAnalyzer::visit(SuperExpr &) {
    if (currentClass.empty()) {
        std::cerr << "Error: 'jilaaka' fuera de clase" << std::endl;
        currentType = "";
    } else {
        currentType = "kasta:" + currentClass;
    }
    lastInputCall = false;
}

} // namespace aym
