#include "lexer/lexer.h"
#include "parser/parser.h"
#include "codegen/codegen.h"
#include "utils/utils.h"
#include "semantic/semantic.h"
#include "utils/error.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

int main(int argc, char** argv) {
    std::vector<std::string> inputs;
    std::string output;
    bool outputProvided = false;
    bool debug = false;
    bool dumpAst = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            std::cout << "Uso: aymc [opciones] archivo.aym ..." << std::endl;
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            output = argv[++i];
            outputProvided = true;
        } else if (arg == "--debug") {
            debug = true;
        } else if (arg == "--dump-ast") {
            dumpAst = true;
        } else {
            inputs.push_back(arg);
        }
    }

    if (inputs.empty()) {
        aym::error("Se requiere un archivo de entrada");
        return 1;
    }

    if (!outputProvided) {
        std::string base = inputs[0];
        size_t slash = base.find_last_of("/\\");
        if (slash != std::string::npos) base = base.substr(slash + 1);
        size_t dot = base.find_last_of('.');
        if (dot != std::string::npos) base = base.substr(0, dot);
        output = "build/" + base;
    }

    std::string source;
    for (const auto &in : inputs) {
        std::ifstream file(in);
        if (!file.is_open()) {
            aym::error("No se pudo abrir el archivo: " + in);
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        source += buffer.str();
        source += "\n";
    }
    aym::Lexer lexer(source);
    auto tokens = lexer.tokenize();
    if (debug) {
        for (const auto &t : tokens) std::cout << static_cast<int>(t.type) << ":" << t.text << std::endl;
    }

    aym::Parser parser(tokens);
    auto nodes = parser.parse();
    if (parser.hasError()) {
        return 1;
    }
    if (dumpAst) {
        std::cout << "AST nodos: " << nodes.size() << std::endl;
    }
    aym::SemanticAnalyzer sem;
    sem.analyze(nodes);

    aym::CodeGenerator cg;
    cg.generate(nodes, output + ".asm", sem.getGlobals(), sem.getParamTypes(), sem.getGlobalTypes());

    return 0;
}
