#include "semantic.h"

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

} // namespace aym
