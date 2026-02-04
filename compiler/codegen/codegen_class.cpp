#include "codegen_impl.h"
#include "../utils/class_names.h"

#include <algorithm>

namespace aym {

void CodeGenImpl::registerClass(const ClassStmt *cls) {
    classes[cls->getName()] = cls;
    for (const auto &method : cls->getMethods()) {
        FunctionInfo info;
        info.name = method.isStatic
            ? classStaticMethodName(cls->getName(), method.name)
            : classMethodName(cls->getName(), method.name);
        info.params = method.params;
        if (!method.isStatic) {
            info.params.insert(info.params.begin(), Param{"kasta:" + cls->getName(), "Aka"});
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
        info.params.insert(info.params.begin(), Param{"kasta:" + cls->getName(), "Aka"});
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
            if (std::find(strings.begin(), strings.end(), field.name) == strings.end()) {
                strings.push_back(field.name);
            }
        }
        for (const auto &method : cls->getMethods()) {
            if (std::find(strings.begin(), strings.end(), method.name) == strings.end()) {
                strings.push_back(method.name);
            }
        }
        if (!cls->getBase().empty()) {
            auto baseIt = classes.find(cls->getBase());
            if (baseIt != classes.end()) {
                const ClassStmt *base = baseIt->second;
                for (const auto &method : base->getMethods()) {
                    std::string superKey = classSuperKey(method.name);
                    if (std::find(strings.begin(), strings.end(), superKey) == strings.end()) {
                        strings.push_back(superKey);
                    }
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
            if (!method.isStatic) {
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
                if (!method.isStatic) {
                    superMethods[method.name] = classMethodName(base->getName(), method.name);
                }
            }
        }
    }
    size_t mapSize = fields.size() + methods.size() + superMethods.size();
    out << "    mov " << reg1(this->windows) << ", " << mapSize << "\n";
    out << "    call aym_map_new\n";
    out << "    mov r12, rax\n";
    std::vector<std::string> regs = paramRegs(this->windows);
    for (const auto *field : fields) {
        if (field->init) {
            emitExpr(field->init.get(), locals);
        } else {
            out << "    mov rax, 0\n";
        }
        out << "    mov r14, rax\n";
        size_t keyIdx = findString(field->name);
        out << "    lea " << regs[1] << ", [rel str" << keyIdx << "]\n";
        out << "    mov " << regs[2] << ", r14\n";
        out << "    mov " << regs[3] << ", "
            << (field->init && isStringExpr(field->init.get(), locals) ? 1 : 0) << "\n";
        out << "    mov " << regs[0] << ", r12\n";
        out << "    call aym_map_set\n";
    }
    for (const auto &method : methods) {
        size_t keyIdx = findString(method.first);
        out << "    lea " << regs[1] << ", [rel str" << keyIdx << "]\n";
        out << "    lea " << regs[2] << ", [rel " << method.second << "]\n";
        out << "    mov " << regs[3] << ", 0\n";
        out << "    mov " << regs[0] << ", r12\n";
        out << "    call aym_map_set\n";
    }
    for (const auto &method : superMethods) {
        std::string superKey = classSuperKey(method.first);
        size_t keyIdx = findString(superKey);
        out << "    lea " << regs[1] << ", [rel str" << keyIdx << "]\n";
        out << "    lea " << regs[2] << ", [rel " << method.second << "]\n";
        out << "    mov " << regs[3] << ", 0\n";
        out << "    mov " << regs[0] << ", r12\n";
        out << "    call aym_map_set\n";
    }
    bool hasCtor = false;
    for (const auto &ctor : cls->getConstructors()) {
        if (ctor.params.size() == expr->getArgs().size()) {
            hasCtor = true;
            break;
        }
    }
    if (hasCtor) {
        std::string ctorName = classCtorName(cls->getName(), expr->getArgs().size());
        for (size_t i = 0; i < expr->getArgs().size() && i + 1 < regs.size(); ++i) {
            emitExpr(expr->getArgs()[i].get(), locals);
            out << "    mov " << regs[i + 1] << ", rax\n";
        }
        out << "    mov " << regs[0] << ", r12\n";
        out << "    call " << ctorName << "\n";
    }
    out << "    mov rax, r12\n";
    return true;
}

bool CodeGenImpl::emitMemberCallExpr(const MemberCallExpr *expr,
                                    const std::unordered_map<std::string,int> *locals) {
    if (!expr) {
        return false;
    }
    if (!expr->getStaticCallee().empty()) {
        std::vector<std::string> regs = paramRegs(this->windows);
        for (size_t i = 0; i < expr->getArgs().size() && i < regs.size(); ++i) {
            emitExpr(expr->getArgs()[i].get(), locals);
            out << "    mov " << regs[i] << ", rax\n";
        }
        out << "    call " << expr->getStaticCallee() << "\n";
        return true;
    }
    std::vector<std::string> regs = paramRegs(this->windows);
    if (dynamic_cast<const SuperExpr*>(expr->getBase())) {
        if (locals && locals->count("Aka")) {
            out << "    mov rbx, [rbp-" << locals->at("Aka") << "]\n";
        } else {
            out << "    mov rbx, [rel Aka]\n";
        }
        std::string superKey = classSuperKey(expr->getMember());
        size_t keyIdx = findString(superKey);
        out << "    lea " << regs[1] << ", [rel str" << keyIdx << "]\n";
        out << "    mov " << regs[0] << ", rbx\n";
        out << "    call aym_map_get\n";
    } else {
        emitExpr(expr->getBase(), locals);
        out << "    mov rbx, rax\n";
        size_t keyIdx = findString(expr->getMember());
        out << "    lea " << regs[1] << ", [rel str" << keyIdx << "]\n";
        out << "    mov " << regs[0] << ", rbx\n";
        out << "    call aym_map_get\n";
    }
    out << "    mov r14, rax\n";
    out << "    mov r12, rbx\n";
    for (size_t i = 0; i < expr->getArgs().size() && i + 1 < regs.size(); ++i) {
        emitExpr(expr->getArgs()[i].get(), locals);
        out << "    mov " << regs[i + 1] << ", rax\n";
    }
    out << "    mov " << regs[0] << ", r12\n";
    out << "    call r14\n";
    return true;
}

bool CodeGenImpl::emitSuperExpr(const SuperExpr *,
                               const std::unordered_map<std::string,int> *locals) {
    if (locals && locals->count("Aka")) {
        out << "    mov rax, [rbp-" << locals->at("Aka") << "]\n";
    } else {
        out << "    mov rax, [rel Aka]\n";
    }
    return true;
}

} // namespace aym
