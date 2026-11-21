// Optimizador de mirilla (peephole) sobre el ASM NASM generado.
// Se desactiva con la variable de entorno AYM_NO_PEEPHOLE=1.
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace aym {

namespace {

const std::string kMovRaxPrefix = "    mov rax, ";

struct CondJumpEntry {
    const char *setLine;
    const char *jmpTrue;
    const char *jmpFalse;
};

const CondJumpEntry kCondJumpTable[] = {
    {"    setl al",  "jl",  "jge"},
    {"    setle al", "jle", "jg"},
    {"    setg al",  "jg",  "jle"},
    {"    setge al", "jge", "jl"},
    {"    sete al",  "je",  "jne"},
    {"    setne al", "jne", "je"},
};

// Operando simple: entero inmediato, [rbp+-N] o [rel simbolo].
// No depende de rax ni de rsp.
bool isSimpleOperand(const std::string &op) {
    if (op.empty()) return false;
    size_t i = 0;
    if (op[0] == '-') i = 1;
    if (i < op.size()) {
        bool allDigits = true;
        for (size_t k = i; k < op.size(); ++k) {
            if (!std::isdigit(static_cast<unsigned char>(op[k]))) { allDigits = false; break; }
        }
        if (allDigits) return true;
    }
    if (op.size() >= 7 && op.compare(0, 4, "[rbp") == 0 && (op[4] == '-' || op[4] == '+') && op.back() == ']') {
        for (size_t k = 5; k + 1 < op.size(); ++k) {
            if (!std::isdigit(static_cast<unsigned char>(op[k]))) return false;
        }
        return true;
    }
    if (op.size() > 6 && op.compare(0, 5, "[rel ") == 0 && op.back() == ']') {
        for (size_t k = 5; k + 1 < op.size(); ++k) {
            if (std::isspace(static_cast<unsigned char>(op[k]))) return false;
        }
        return true;
    }
    return false;
}

} // namespace

void peepholeOptimizeAsmFile(const std::string &path) {
    const char *off = std::getenv("AYM_NO_PEEPHOLE");
    if (off != nullptr && off[0] != '\0' && off[0] != '0') return;

    std::vector<std::string> lines;
    {
        std::ifstream in(path);
        if (!in.is_open()) return;
        std::string line;
        while (std::getline(in, line)) lines.push_back(line);
    }

    bool everChanged = false;
    bool changed = true;
    while (changed) {
        changed = false;
        std::vector<std::string> result;
        result.reserve(lines.size());
        const size_t n = lines.size();
        for (size_t i = 0; i < n; ++i) {
            // push rax / sub rsp, (8|40) / mov rax, X / add rsp, (8|40) / mov rbx, rax / pop rax
            //   =>  mov rbx, X      (X inmediato, [rbp+-N] o [rel simbolo])
            if (i + 5 < n && lines[i] == "    push rax" &&
                ((lines[i + 1] == "    sub rsp, 8" && lines[i + 3] == "    add rsp, 8") ||
                 (lines[i + 1] == "    sub rsp, 40" && lines[i + 3] == "    add rsp, 40")) &&
                lines[i + 2].compare(0, kMovRaxPrefix.size(), kMovRaxPrefix) == 0 &&
                lines[i + 4] == "    mov rbx, rax" &&
                lines[i + 5] == "    pop rax") {
                const std::string operand = lines[i + 2].substr(kMovRaxPrefix.size());
                if (isSimpleOperand(operand)) {
                    result.push_back("    mov rbx, " + operand);
                    i += 5;
                    changed = true;
                    continue;
                }
            }
            // set<cc> al / movzx rax,al / cmp rax,0 / je|jne <L>
            //   =>  j<cond> <L>
            if (i + 3 < n && lines[i + 1] == "    movzx rax,al" && lines[i + 2] == "    cmp rax,0") {
                const bool isJe = lines[i + 3].compare(0, 7, "    je ") == 0;
                const bool isJne = !isJe && lines[i + 3].compare(0, 8, "    jne ") == 0;
                if (isJe || isJne) {
                    const std::string target = lines[i + 3].substr(isJe ? 7 : 8);
                    if (!target.empty()) {
                        const char *replacement = nullptr;
                        for (const auto &entry : kCondJumpTable) {
                            if (lines[i] == entry.setLine) {
                                replacement = isJne ? entry.jmpTrue : entry.jmpFalse;
                                break;
                            }
                        }
                        if (replacement != nullptr) {
                            result.push_back(std::string("    ") + replacement + " " + target);
                            i += 3;
                            changed = true;
                            continue;
                        }
                    }
                }
            }
            // jmp L seguido inmediatamente de L:
            if (i + 1 < n && lines[i].compare(0, 8, "    jmp ") == 0) {
                const std::string target = lines[i].substr(8);
                if (!target.empty() && lines[i + 1] == target + ":") {
                    changed = true;
                    continue;
                }
            }
            result.push_back(lines[i]);
        }
        lines.swap(result);
        if (changed) {
            everChanged = true;
        }
    }

    if (!everChanged) return;
    std::ofstream outFile(path, std::ios::trunc);
    if (!outFile.is_open()) return;
    for (const auto &l : lines) outFile << l << "\n";
}

} // namespace aym
