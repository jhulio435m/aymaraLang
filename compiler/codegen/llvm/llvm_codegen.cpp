#include "llvm_codegen.h"

#include "../../ast/ast.h"

#ifdef AYM_WITH_LLVM

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include <sstream>
#include <stdexcept>
#include <vector>

namespace aym {

namespace {
std::string buildSummaryMessage(const std::vector<std::unique_ptr<Node>> &nodes,
                                const std::unordered_set<std::string> &globals,
                                const std::unordered_map<std::string, std::vector<std::string>> &paramTypes,
                                const std::unordered_map<std::string, std::string> &globalTypes,
                                long seed) {
    size_t functionCount = 0;
    size_t statementCount = 0;
    for (const auto &node : nodes) {
        if (!node) continue;
        if (dynamic_cast<const FunctionStmt *>(node.get())) {
            ++functionCount;
        } else if (dynamic_cast<const Stmt *>(node.get())) {
            ++statementCount;
        }
    }

    std::ostringstream oss;
    oss << "AymaraLang LLVM backend (experimental)\n";
    oss << " - Top-level statements: " << statementCount << "\n";
    oss << " - Functions: " << functionCount << "\n";
    oss << " - Declared globals: " << globals.size() << "\n";
    oss << " - Annotated global types: " << globalTypes.size() << "\n";
    oss << " - Functions with parameter metadata: " << paramTypes.size() << "\n";
    if (seed >= 0) {
        oss << " - Random seed: " << seed << "\n";
    }
    oss << "Este backend genera IR ilustrativo; amplía la implementación para un compilador completo.";
    return oss.str();
}
} // namespace

void LLVMCodeGenerator::generate(const std::vector<std::unique_ptr<Node>> &nodes,
                                 const std::string &outputPath,
                                 const std::unordered_set<std::string> &globals,
                                 const std::unordered_map<std::string, std::vector<std::string>> &paramTypes,
                                 const std::unordered_map<std::string, std::string> &globalTypes,
                                 long seed) {
    llvm::LLVMContext context;
    auto module = std::make_unique<llvm::Module>("aymara", context);
    llvm::IRBuilder<> builder(context);

    auto *int32Ty = builder.getInt32Ty();
    auto *int8PtrTy = llvm::PointerType::get(builder.getInt8Ty(), 0);
    std::vector<llvm::Type *> printfArgs;
    printfArgs.push_back(int8PtrTy);
    auto printfType = llvm::FunctionType::get(int32Ty, printfArgs, true);
    auto printfFunc = module->getOrInsertFunction("printf", printfType);

    auto mainType = llvm::FunctionType::get(int32Ty, false);
    auto *mainFunc = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", module.get());
    auto *entry = llvm::BasicBlock::Create(context, "entry", mainFunc);
    builder.SetInsertPoint(entry);

    std::string message = buildSummaryMessage(nodes, globals, paramTypes, globalTypes, seed);
    auto *messageValue = builder.CreateGlobalStringPtr(message, "aym_summary");
    builder.CreateCall(printfFunc, {messageValue});
    builder.CreateRet(builder.getInt32(0));

    std::error_code ec;
    llvm::raw_fd_ostream dest(outputPath, ec, llvm::sys::fs::OF_Text);
    if (ec) {
        throw std::runtime_error("No se pudo abrir el archivo de salida LLVM: " + ec.message());
    }

    module->print(dest, nullptr);
}

} // namespace aym

#else // !AYM_WITH_LLVM

#include "../../utils/error.h"

namespace aym {

void LLVMCodeGenerator::generate(const std::vector<std::unique_ptr<Node>> &nodes,
                                 const std::string &outputPath,
                                 const std::unordered_set<std::string> &globals,
                                 const std::unordered_map<std::string, std::vector<std::string>> &paramTypes,
                                 const std::unordered_map<std::string, std::string> &globalTypes,
                                 long seed) {
    (void)nodes;
    (void)outputPath;
    (void)globals;
    (void)paramTypes;
    (void)globalTypes;
    (void)seed;
    error("El backend LLVM no está disponible en esta compilación. Recompila con soporte de LLVM.");
}

} // namespace aym

#endif
