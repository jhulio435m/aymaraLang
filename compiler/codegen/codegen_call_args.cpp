#include "codegen_impl.h"

#include <algorithm>

namespace aym {

void CodeGenImpl::emitCallArgs(const std::vector<std::unique_ptr<Expr>> &args,
                               const std::unordered_map<std::string,int> *locals,
                               size_t regStart) {
    std::vector<std::string> regs = paramRegs(this->windows);
    if (regStart >= regs.size() || args.empty()) {
        return;
    }

    size_t count = std::min(args.size(), regs.size() - regStart);
    // On Win64, nested calls can use the 32-byte shadow space at [rsp..rsp+31].
    // Keep staged arguments above that area to avoid them being clobbered.
    size_t shadow = this->windows ? 32 : 0;
    size_t bytes = count * 8;
    size_t frame = (shadow + bytes + 15) & ~static_cast<size_t>(15);
    if (frame > 0) {
        out << "    sub rsp, " << frame << "\n";
    }

    for (size_t i = 0; i < count; ++i) {
        emitExpr(args[i].get(), locals);
        out << "    mov [rsp+" << (shadow + i * 8) << "], rax\n";
    }

    for (size_t i = 0; i < count; ++i) {
        out << "    mov " << regs[regStart + i] << ", [rsp+" << (shadow + i * 8) << "]\n";
    }

    if (frame > 0) {
        out << "    add rsp, " << frame << "\n";
    }
}

} // namespace aym
