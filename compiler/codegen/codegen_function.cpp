#include "codegen_impl.h"
#include <stdexcept>

namespace aym {

void CodeGenImpl::emitFunction(const FunctionInfo &info) {
    std::unordered_map<std::string,int> offsets;
    // We save rbx/r12-r15 in the prologue, so locals start below those slots.
    constexpr int savedRegsBytes = 5 * 8;
    int off = savedRegsBytes;
    for (const auto &n : info.locals) {
        off += 8;
        offsets[n] = off;
    }
    int localBytes = off - savedRegsBytes;
    // Reserve Win64 shadow space (32 bytes) so calls are ABI-compliant.
    int shadow = this->windows ? 32 : 0;
    int needed = localBytes + shadow;
    // After push rbp + 5 callee-saved registers, rsp is misaligned by 8.
    // Keep stackSize == 8 (mod 16) so rsp stays 16-byte aligned at call sites.
    int stackSize = ((needed + 15) & ~15) + 8;

    currentParamStrings.clear();
    currentParamTypes.clear();
    currentLocalStrings = info.stringLocals;
    currentLocalTypes = info.localTypes;
    auto pit = paramTypes.find(info.name);
    if (pit != paramTypes.end()) {
        size_t idx = 0;
        for (const auto &p : info.params) {
            if (idx < pit->second.size()) {
                currentParamTypes[p.name] = pit->second[idx];
                if (pit->second[idx] == "aru") currentParamStrings[p.name] = true;
            }
            ++idx;
        }
    }

    std::string endLabel = genLabel("endfunc");

    out << info.name << ":\n";
    out << "    push rbp\n";
    out << "    mov rbp, rsp\n";
    // Callee-saved registers used by generated code.
    out << "    push rbx\n";
    out << "    push r12\n";
    out << "    push r13\n";
    out << "    push r14\n";
    out << "    push r15\n";
    if (stackSize) out << "    sub rsp, " << stackSize << "\n";

    // Limite de pila: recursion demasiado profunda lanza la excepcion PILA.
    {
        std::string stackOk = genLabel("stack_ok");
        out << "    cmp rsp, [rel aym_stack_limit]\n";
        out << "    jae " << stackOk << "\n";
        out << "    call aym_stack_overflow\n";
        out << stackOk << ":\n";
    }

    // store parameters
    std::vector<std::string> regs = paramRegs(this->windows);
    size_t idx = 0;
    for (const auto &p : info.params) {
        if (idx < regs.size()) {
            out << "    mov [rbp-" << offsets[p.name] << "], " << regs[idx] << "\n";
        } else {
            // Parametro pasado en la pila del llamador: [rbp+16] tras la
            // direccion de retorno (+32 de shadow space en Win64).
            int src = 16 + shadow + static_cast<int>((idx - regs.size()) * 8);
            out << "    mov rax, [rbp+" << src << "]\n";
            out << "    mov [rbp-" << offsets[p.name] << "], rax\n";
        }
        ++idx;
    }

    // Con modulos, cada funcion marca su archivo; el llamador lo restaura al
    // volver (emitRestoreLocation).
    std::string prevFile = currentFile;
    size_t prevLine = currentStmtLine;
    currentFile = info.sourceFile;
    currentStmtLine = 0;
    if (multiFile && !info.sourceFile.empty()) emitSetFile(info.sourceFile);

    emitStmt(info.body, &offsets, endLabel);

    if (pendingStackArgs != 0) {
        // Un sitio de llamada no libero sus argumentos en pila: el codigo
        // generado tendria la pila desfasada.
        throw std::runtime_error("error interno del compilador: argumentos en pila sin liberar en '" +
                                 info.name + "'");
    }

    // Si la ejecucion llega al final sin 'kuttaya', el valor devuelto es 0
    // (y no basura de rax). Los 'kuttaya' saltan directo a endLabel.
    out << "    xor eax, eax\n";
    out << endLabel << ":\n";
    if (stackSize) out << "    add rsp, " << stackSize << "\n";
    out << "    pop r15\n";
    out << "    pop r14\n";
    out << "    pop r13\n";
    out << "    pop r12\n";
    out << "    pop rbx\n";
    out << "    pop rbp\n";
    out << "    ret\n";
    currentFile = prevFile;
    currentStmtLine = prevLine;
}

} // namespace aym
