#include "lexer/lexer.h"
#include "parser/parser.h"
#include "codegen/codegen.h"
#ifdef AYM_WITH_LLVM
#include "codegen/llvm/llvm_codegen.h"
#endif
#include "interpreter/interpreter.h"
#include "utils/utils.h"
#include "semantic/semantic.h"
#include "utils/error.h"
#include "utils/module_resolver.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "utils/fs.h"

int main(int argc, char** argv) {
    std::vector<std::string> inputs;
    std::string output;
    bool outputProvided = false;
    bool debug = false;
    bool dumpAst = false;
    bool repl = false;
    bool windowsTarget =
#ifdef _WIN32
        true;
#else
        false;
#endif
    long seed = 0;
    bool seedProvided = false;
    bool useLLVMBackend = false;

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
        } else if (arg == "--repl") {
            repl = true;
        } else if (arg == "--windows") {
            windowsTarget = true;
        } else if (arg == "--linux") {
            windowsTarget = false;
        } else if (arg == "--seed" && i + 1 < argc) {
            seed = std::stol(argv[++i]);
            seedProvided = true;
        } else if (arg == "--llvm") {
#ifdef AYM_WITH_LLVM
            useLLVMBackend = true;
#else
            aym::error("El backend LLVM no está disponible en esta compilación. Recompila con soporte de LLVM.");
            return 1;
#endif
        } else {
            inputs.push_back(arg);
        }
    }

    if (repl) {
        std::cout << "AymaraLang REPL - escribe código línea por línea (escribe 'salir' para terminar)" << std::endl;
        aym::Interpreter interp;
        interp.setModuleBase(fs::current_path());
        if (seedProvided) interp.setSeed(static_cast<unsigned int>(seed));
        std::vector<std::unique_ptr<aym::Node>> program;
        aym::SemanticAnalyzer sem;
        aym::ModuleResolver resolver(fs::current_path());
        std::string line;
        while (true) {
            std::cout << "aym> ";
            if (!std::getline(std::cin, line)) break;
            if (line == "salir" || line == "exit") break;
            bool exprOnly = (line.find(';') == std::string::npos);
            std::string src = line;
            if (exprOnly) src += ";";
            aym::Lexer lx(src);
            auto toks = lx.tokenize();
            aym::Parser p(toks);
            auto nodes = p.parse();
            if (p.hasError()) continue;
            resolver.resolve(nodes, fs::current_path());
            size_t start = program.size();
            for (auto &n : nodes) program.push_back(std::move(n));
            sem.analyze(program);
            for (size_t i = start; i < program.size(); ++i) {
                program[i]->accept(interp);
            }
            if (exprOnly && !nodes.empty()) {
                auto val = interp.getLastValue();
                if (val.type == aym::Value::Type::String)
                    std::cout << val.s << std::endl;
                else if (val.type == aym::Value::Type::Int)
                    std::cout << val.i << std::endl;
            }
        }
        return 0;
    }

    if (inputs.empty()) {
        aym::error("Se requiere un archivo de entrada");
        return 1;
    }

    if (!outputProvided) {
        fs::path base = fs::path(inputs[0]).stem();
        output = (fs::path("build") / base).string();
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
    fs::path entryDir = fs::current_path();
    if (!inputs.empty()) {
        fs::path first = fs::path(inputs[0]);
        if (!first.is_absolute()) first = fs::absolute(first);
        if (fs::is_directory(first)) entryDir = first;
        else if (first.has_parent_path()) entryDir = first.parent_path();
    }
    aym::ModuleResolver resolver(entryDir);
    for (const auto &in : inputs) {
        fs::path p = fs::path(in);
        if (!p.is_absolute()) p = fs::absolute(p);
        if (p.has_parent_path()) resolver.addSearchPath(p.parent_path());
    }
    resolver.resolve(nodes, entryDir);
    if (dumpAst) {
        std::cout << "AST nodos: " << nodes.size() << std::endl;
    }
    aym::SemanticAnalyzer sem;
    sem.analyze(nodes);

#ifdef AYM_WITH_LLVM
    if (useLLVMBackend) {
        aym::LLVMCodeGenerator llvmCg;
        llvmCg.generate(nodes,
                        output + ".ll",
                        sem.getGlobals(),
                        sem.getParamTypes(),
                        sem.getGlobalTypes(),
                        seedProvided ? seed : -1);
        return 0;
    }
#else
    (void)useLLVMBackend; // suprimir advertencias cuando LLVM está deshabilitado
#endif

    aym::CodeGenerator cg;
    cg.generate(nodes,
                output + ".asm",
                sem.getGlobals(),
                sem.getParamTypes(),
                sem.getGlobalTypes(),
                windowsTarget,
                seedProvided ? seed : -1);

    return 0;
}
