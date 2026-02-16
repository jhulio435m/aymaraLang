#ifndef AYM_BACKEND_H
#define AYM_BACKEND_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../ast/ast.h"
#include "../codegen/codegen.h"

namespace aym {

enum class BackendKind {
    Native,
    Ir
};

bool parseBackendKind(const std::string &value, BackendKind &kind, std::string &errorMessage);
const char *backendKindName(BackendKind kind);

bool runBackendCompile(BackendKind kind,
                       const std::vector<std::unique_ptr<Node>> &nodes,
                       const std::string &outputPath,
                       const std::unordered_set<std::string> &globals,
                       const std::unordered_map<std::string, std::vector<std::string>> &paramTypes,
                       const std::unordered_map<std::string, std::string> &functionReturnTypes,
                       const std::unordered_map<std::string, std::string> &globalTypes,
                       bool windowsTarget,
                       long seed,
                       const std::string &runtimeDir,
                       bool keepAsm,
                       CodegenPipelineMode mode,
                       bool timePipeline,
                       const std::string &timePipelineJsonPath,
                       long long toolTimeoutMs,
                       std::string &errorMessage);

} // namespace aym

#endif // AYM_BACKEND_H
