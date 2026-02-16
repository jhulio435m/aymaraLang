#include "codegen_impl.h"
#include "../builtins/builtins.h"

namespace aym {

void CodeGenImpl::emitCallExpr(const CallExpr *c,
                               const std::unordered_map<std::string,int> *locals) {
    std::string nameLower = lowerName(c->getName());
    if (emitBuiltinIoCall(c, locals, nameLower) ||
        emitBuiltinStringCall(c, locals, nameLower) ||
        emitBuiltinCollectionCall(c, locals, nameLower) ||
        emitBuiltinSystemCall(c, locals, nameLower) ||
        emitBuiltinFunctionalCall(c, locals, nameLower) ||
        emitBuiltinFsCall(c, locals, nameLower) ||
        emitBuiltinArrayPrimitiveCall(c, locals, nameLower)) {
        return;
    }

    // user function call
    emitCallArgs(c->getArgs(), locals, 0);
    out << "    call " << c->getName() << "\n";
}

} // namespace aym
