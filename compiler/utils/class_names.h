#ifndef AYM_CLASS_NAMES_H
#define AYM_CLASS_NAMES_H

#include <string>

namespace aym {

inline std::string classMethodName(const std::string &className, const std::string &method) {
    return "__" + className + "_" + method;
}

inline std::string classStaticMethodName(const std::string &className, const std::string &method) {
    return "__" + className + "_static_" + method;
}

inline std::string classCtorName(const std::string &className, size_t arity) {
    return "__" + className + "_ctor_" + std::to_string(arity);
}

inline std::string classStaticFieldName(const std::string &className, const std::string &field) {
    return "__" + className + "_static_" + field;
}

inline std::string classSuperKey(const std::string &method) {
    return "super::" + method;
}

} // namespace aym

#endif // AYM_CLASS_NAMES_H
