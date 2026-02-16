#ifndef AYM_CODEGEN_H
#define AYM_CODEGEN_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace aym {

class Node;

enum class CodegenPipelineMode {
    Full,
    CompileOnly,
    LinkOnly
};

class CodeGenerator {
public:
    bool generate(const std::vector<std::unique_ptr<Node>> &nodes,
                  const std::string &outputPath,
                  const std::unordered_set<std::string> &globals,
                  const std::unordered_map<std::string, std::vector<std::string>> &paramTypes,
                  const std::unordered_map<std::string, std::string> &functionReturnTypes,
                  const std::unordered_map<std::string, std::string> &globalTypes,
                  bool windows,
                  long seed,
                  const std::string &runtimeDir,
                  bool keepAsm = false,
                  CodegenPipelineMode mode = CodegenPipelineMode::Full,
                  bool timePipeline = false,
                  const std::string &timePipelineJsonPath = "",
                  long long toolTimeoutMs = 0,
                  std::string *errorMessage = nullptr);
};

} // namespace aym

#endif // AYM_CODEGEN_H
