#include "lexer/lexer.h"
#include "parser/parser.h"
#include "codegen/codegen.h"
#include "utils/utils.h"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Uso: aymc <archivo.aym>" << std::endl;
        return 1;
    }

    std::string source = aym::readFile(argv[1]);
    aym::Lexer lexer(source);
    auto tokens = lexer.tokenize();

    aym::Parser parser(tokens);
    parser.parse();

    aym::CodeGenerator cg;
    cg.generate();

    return 0;
}
