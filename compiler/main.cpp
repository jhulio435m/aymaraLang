#include "lexer/lexer.h"
#include "parser/parser.h"
#include "codegen/codegen.h"
#include "utils/utils.h"
#include "semantic/semantic.h"
#include "utils/error.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::string input;
    std::string output = "build/out";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            std::cout << "Uso: aymc [-o salida] archivo.aym" << std::endl;
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            output = argv[++i];
        } else {
            input = arg;
        }
    }

    if (input.empty()) {
        aym::error("Se requiere un archivo de entrada");
        return 1;
    }

    std::string source = aym::readFile(input);
    aym::Lexer lexer(source);
    auto tokens = lexer.tokenize();

    aym::Parser parser(tokens);
    auto nodes = parser.parse();

    aym::SemanticAnalyzer sem;
    sem.analyze(nodes);

    aym::CodeGenerator cg;
    cg.generate(nodes, output + ".asm");

    return 0;
}
