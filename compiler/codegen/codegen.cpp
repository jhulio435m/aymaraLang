#include "codegen_impl.h"

namespace aym {

bool CodeGenerator::generate(const std::vector<std::unique_ptr<Node>> &nodes,
                             const std::string &outputPath,
                             const std::unordered_set<std::string> &globals,
                             const std::unordered_map<std::string,std::vector<std::string>> &paramTypes,
                             const std::unordered_map<std::string,std::string> &functionReturnTypes,
                             const std::unordered_map<std::string,std::string> &globalTypes,
                             bool windows,
                             long seed,
                             const std::string &runtimeDir,
                             bool keepAsm,
                             CodegenPipelineMode mode,
                             bool timePipeline,
                             const std::string &timePipelineJsonPath,
                             long long toolTimeoutMs,
                             std::string *errorMessage) {
    CodeGenImpl impl;
    return impl.emit(nodes,
                     outputPath,
                     globals,
                     paramTypes,
                     functionReturnTypes,
                     globalTypes,
                     windows,
                     seed,
                     runtimeDir,
                     keepAsm,
                     mode,
                     timePipeline,
                     timePipelineJsonPath,
                     toolTimeoutMs,
                     errorMessage);
}

} // namespace aym
