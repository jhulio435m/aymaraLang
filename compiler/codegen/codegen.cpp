#include "codegen.h"
#include "../ast/ast.h"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <memory>

namespace aym {

void CodeGenerator::generate(const std::vector<std::unique_ptr<Node>> &nodes,
                             const std::string &outputPath) {
    std::ofstream asmFile(outputPath);
    asmFile << "section .data\n";

    int idx = 0;
    for (const auto &n : nodes) {
        if (auto *p = dynamic_cast<PrintNode *>(n.get())) {
            asmFile << "msg" << idx << ": db \"" << p->getText() << "\",10\n";
            asmFile << "len" << idx << " equ $-msg" << idx << "\n";
            idx++;
        }
    }

    asmFile << "section .text\n";
    asmFile << "global _start\n";
    asmFile << "_start:\n";

    idx = 0;
    for (const auto &n : nodes) {
        if (dynamic_cast<PrintNode *>(n.get())) {
            asmFile << "    mov rax, 1\n";
            asmFile << "    mov rdi, 1\n";
            asmFile << "    mov rsi, msg" << idx << "\n";
            asmFile << "    mov rdx, len" << idx << "\n";
            asmFile << "    syscall\n";
            idx++;
        }
    }

    asmFile << "    mov rax, 60\n";
    asmFile << "    xor rdi, rdi\n";
    asmFile << "    syscall\n";

    asmFile.close();

    std::string obj = outputPath.substr(0, outputPath.find_last_of('.')) + ".o";
    std::string bin = outputPath.substr(0, outputPath.find_last_of('.'));
    std::string cmd1 = "nasm -felf64 " + outputPath + " -o " + obj;
    std::string cmd2 = "ld " + obj + " -o " + bin;
    if (std::system(cmd1.c_str()) != 0 || std::system(cmd2.c_str()) != 0) {
        std::cerr << "Error assembling or linking" << std::endl;
    }
}

} // namespace aym