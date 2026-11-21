#include "codegen_impl.h"
#include "../utils/class_names.h"

#include <algorithm>

namespace aym {

void CodeGenImpl::registerClass(const ClassStmt *cls) {
    classes[cls->getName()] = cls;
    for (const auto &method : cls->getMethods()) {
        if (method.isAbstract || !method.body) {
            continue;
        }
        FunctionInfo info;
        info.name = method.isStatic
            ? classStaticMethodName(cls->getName(), method.name)
            : classMethodName(cls->getName(), method.name);
        info.params = method.params;
        if (!method.isStatic) {
            info.params.insert(info.params.begin(), Param{"kasta:" + cls->getName(), "Aka", nullptr});
        }
        info.body = method.body.get();
        assignTrySlots(method.body.get());
        for (const auto &param : info.params) {
            info.locals.push_back(param.name);
            info.stringLocals[param.name] = false;
        }
        collectLocals(method.body.get(), info.locals, info.stringLocals, info.localTypes);
        functions.push_back(std::move(info));
    }
    for (const auto &ctor : cls->getConstructors()) {
        FunctionInfo info;
        info.name = classCtorName(cls->getName(), ctor.params.size());
        info.params = ctor.params;
        info.params.insert(info.params.begin(), Param{"kasta:" + cls->getName(), "Aka", nullptr});
        info.body = ctor.body.get();
        assignTrySlots(ctor.body.get());
        for (const auto &param : info.params) {
            info.locals.push_back(param.name);
            info.stringLocals[param.name] = false;
        }
        collectLocals(ctor.body.get(), info.locals, info.stringLocals, info.localTypes);
        functions.push_back(std::move(info));
    }
}

void CodeGenImpl::collectClassStrings() {
    for (const auto &pair : classes) {
        const ClassStmt *cls = pair.second;
        for (const auto &field : cls->getFields()) {
            collectStrings(field.init.get());
            registerString(field.name);
        }
        for (const auto &method : cls->getMethods()) {
            registerString(method.name);
        }
        if (!cls->getBase().empty()) {
            auto baseIt = classes.find(cls->getBase());
            if (baseIt != classes.end()) {
                const ClassStmt *base = baseIt->second;
                for (const auto &method : base->getMethods()) {
                    std::string superKey = classSuperKey(method.name);
                    registerString(superKey);
                }
            }
        }
    }
}

bool CodeGenImpl::emitNewExpr(const NewExpr *expr,
                             const std::unordered_map<std::string,int> *locals) {
    auto it = classes.find(expr->getName());
    if (it == classes.end()) {
        out << "    mov rax,0\n";
        return true;
    }
    const ClassStmt *cls = it->second;
    std::vector<const ClassStmt*> lineage;
    for (const ClassStmt *c = cls; c; ) {
        lineage.push_back(c);
        if (c->getBase().empty()) break;
        auto baseIt = classes.find(c->getBase());
        if (baseIt == classes.end()) break;
        c = baseIt->second;
    }
    std::reverse(lineage.begin(), lineage.end());
    std::vector<const ClassStmt::FieldDecl*> fields;
    std::unordered_map<std::string, std::string> methods;
    for (const auto *c : lineage) {
        for (const auto &field : c->getFields()) {
            if (!field.isStatic) {
                fields.push_back(&field);
            }
        }
        for (const auto &method : c->getMethods()) {
            if (!method.isStatic && !method.isAbstract) {
                methods[method.name] = classMethodName(c->getName(), method.name);
            }
        }
    }
    std::unordered_map<std::string, std::string> superMethods;
    if (!cls->getBase().empty()) {
        auto baseIt = classes.find(cls->getBase());
        if (baseIt != classes.end()) {
            const ClassStmt *base = baseIt->second;
            for (const auto &method : base->getMethods()) {
                if (!method.isStatic && !method.isAbstract) {
                    superMethods[method.name] = classMethodName(base->getName(), method.name);
                }
            }
        }
    }
    size_t mapSize = fields.size() + methods.size() + superMethods.size();
    int spillPad = this->windows ? 40 : 8;
    out << "    mov " << reg1(this->windows) << ", " << mapSize << "\n";
    out << "    call aym_map_new\n";
    out << "    push rax\n";
    out << "    sub rsp, " << spillPad << "\n";
    std::vector<std::string> regs = paramRegs(this->windows);
    for (const auto *field : fields) {
        if (field->init) {
            emitExpr(field->init.get(), locals);
        } else {
            out << "    mov rax, 0\n";
        }
        out << "    mov " << regs[2] << ", rax\n";
        size_t keyIdx = findString(field->name);
        out << "    lea " << regs[1] << ", [rel str" << keyIdx << "]\n";
        out << "    mov " << regs[3] << ", "
            << (field->init && isStringExpr(field->init.get(), locals) ? 1 : 0) << "\n";
        out << "    mov " << regs[0] << ", [rsp+" << spillPad << "]\n";
        out << "    call aym_map_set\n";
    }
    for (const auto &method : methods) {
        size_t keyIdx = findString(method.first);
        out << "    lea " << regs[1] << ", [rel str" << keyIdx << "]\n";
        out << "    lea " << regs[2] << ", [rel " << method.second << "]\n";
        out << "    mov " << regs[3] << ", 0\n";
        out << "    mov " << regs[0] << ", [rsp+" << spillPad << "]\n";
        out << "    call aym_map_set\n";
    }
    for (const auto &method : superMethods) {
        std::string superKey = classSuperKey(method.first);
        size_t keyIdx = findString(superKey);
        out << "    lea " << regs[1] << ", [rel str" << keyIdx << "]\n";
        out << "    lea " << regs[2] << ", [rel " << method.second << "]\n";
        out << "    mov " << regs[3] << ", 0\n";
        out << "    mov " << regs[0] << ", [rsp+" << spillPad << "]\n";
        out << "    call aym_map_set\n";
    }
    // Sin constructores propios se usa el del ancestro mas cercano que los
    // defina (lineage va de la raiz a la clase instanciada).
    const ClassStmt *ctorOwner = nullptr;
    for (auto lit = lineage.rbegin(); lit != lineage.rend(); ++lit) {
        if (!(*lit)->getConstructors().empty()) {
            ctorOwner = *lit;
            break;
        }
    }
    bool hasCtor = false;
    if (ctorOwner) {
        for (const auto &ctor : ctorOwner->getConstructors()) {
            if (ctor.params.size() == expr->getArgs().size()) {
                hasCtor = true;
                break;
            }
        }
    }
    if (hasCtor) {
        std::string ctorName = classCtorName(ctorOwner->getName(), expr->getArgs().size());
        size_t stackArgs = emitCallArgs(expr->getArgs(), locals, 1);
        out << "    mov " << regs[0] << ", [rsp+" << (stackArgs + spillPad) << "]\n";
        out << "    call " << ctorName << "\n";
        releaseStackArgs(stackArgs);
    }
    out << "    add rsp, " << spillPad << "\n";
    out << "    pop rax\n";
    return true;
}

bool CodeGenImpl::emitMemberCallExpr(const MemberCallExpr *expr,
                                    const std::unordered_map<std::string,int> *locals) {
    if (!expr) {
        return false;
    }
    if (dynamic_cast<const SuperExpr*>(expr->getBase())) {
        // jilaaka.m(...) / jilaaka.qallta(...): llamada directa (resuelta en el
        // analisis semantico) con 'aka' como receptor.
        if (expr->getStaticCallee().empty()) {
            out << "    mov rax, 0\n";
            return true;
        }
        std::vector<std::string> sregs = paramRegs(this->windows);
        size_t stackArgs = emitCallArgs(expr->getArgs(), locals, 1);
        if (locals && locals->count("Aka")) {
            out << "    mov " << sregs[0] << ", [rbp-" << locals->at("Aka") << "]\n";
        } else {
            out << "    mov " << sregs[0] << ", [rel $Aka]\n";
        }
        out << "    call " << expr->getStaticCallee() << "\n";
        releaseStackArgs(stackArgs);
        return true;
    }
    if (!expr->getStaticCallee().empty()) {
        size_t stackArgs = emitCallArgs(expr->getArgs(), locals, 0);
        out << "    call " << expr->getStaticCallee() << "\n";
        releaseStackArgs(stackArgs);
        return true;
    }
    int spillPad = this->windows ? 40 : 8;
    std::vector<std::string> regs = paramRegs(this->windows);
    if (dynamic_cast<const SuperExpr*>(expr->getBase())) {
        if (locals && locals->count("Aka")) {
            out << "    mov rax, [rbp-" << locals->at("Aka") << "]\n";
        } else {
            out << "    mov rax, [rel $Aka]\n";
        }
        out << "    push rax\n";
        out << "    sub rsp, " << spillPad << "\n";
        std::string superKey = classSuperKey(expr->getMember());
        size_t keyIdx = findString(superKey);
        out << "    lea " << regs[1] << ", [rel str" << keyIdx << "]\n";
        out << "    mov " << regs[0] << ", rax\n";
        out << "    call aym_map_get\n";
    } else {
        emitExpr(expr->getBase(), locals);
        out << "    push rax\n";
        out << "    sub rsp, " << spillPad << "\n";
        size_t keyIdx = findString(expr->getMember());
        out << "    lea " << regs[1] << ", [rel str" << keyIdx << "]\n";
        out << "    mov " << regs[0] << ", rax\n";
        out << "    call aym_map_get\n";
    }
    out << "    push rax\n";
    out << "    sub rsp, " << spillPad << "\n";
    size_t stackArgs = emitCallArgs(expr->getArgs(), locals, 1);
    if (stackArgs == 0) {
        out << "    add rsp, " << spillPad << "\n";
        out << "    pop r11\n";
        out << "    add rsp, " << spillPad << "\n";
        out << "    pop " << regs[0] << "\n";
        out << "    call r11\n";
    } else {
        // Los argumentos extra ocupan el tope de la pila: el metodo y el
        // receptor resguardados quedan por encima y se leen sin desapilar.
        // (8 + spillPad) es multiplo de 16, asi que rsp sigue alineado.
        size_t slot = 8 + static_cast<size_t>(spillPad);
        out << "    mov r11, [rsp+" << (stackArgs + spillPad) << "]\n";
        out << "    mov " << regs[0] << ", [rsp+" << (stackArgs + slot + spillPad) << "]\n";
        out << "    call r11\n";
        releaseStackArgs(stackArgs);
        out << "    add rsp, " << (2 * slot) << "\n";
    }
    return true;
}

bool CodeGenImpl::emitSuperExpr(const SuperExpr *,
                               const std::unordered_map<std::string,int> *locals) {
    if (locals && locals->count("Aka")) {
        out << "    mov rax, [rbp-" << locals->at("Aka") << "]\n";
    } else {
        out << "    mov rax, [rel $Aka]\n";
    }
    return true;
}

} // namespace aym
