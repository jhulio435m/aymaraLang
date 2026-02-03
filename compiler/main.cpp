#include "lexer/lexer.h"
#include "parser/parser.h"
#include "codegen/codegen.h"
#ifdef AYM_WITH_LLVM
#include "codegen/llvm/llvm_codegen.h"
#endif
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
        } else if (!arg.empty() && arg[0] == '-') {
            aym::error("Opción no reconocida: " + arg);
            return 1;
        } else {
            inputs.push_back(arg);
        }
    }

    if (inputs.empty()) {
        aym::error("Se requiere un archivo de entrada");
        return 1;
    }

    if (!outputProvided) {
        fs::path inputPath = fs::path(inputs[0]);
        fs::path base = inputPath.stem();
        if (inputPath.has_parent_path()) {
            output = (inputPath.parent_path() / base).string();
        } else {
            output = base.string();
        }
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

    fs::path runtimeDir = fs::path(aym::executableDir()) / "runtime";
    if (!fs::exists(runtimeDir)) {
        runtimeDir = fs::path("runtime");
    }

    aym::CodeGenerator cg;
    cg.generate(nodes,
                output + ".asm",
                sem.getGlobals(),
                sem.getParamTypes(),
                sem.getGlobalTypes(),
                windowsTarget,
                seedProvided ? seed : -1,
                runtimeDir.string());

    return 0;
}
