#ifndef AYM_LLVM_CODEGEN_H
#define AYM_LLVM_CODEGEN_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace aym {

class Node;

class LLVMCodeGenerator {
public:
    void generate(const std::vector<std::unique_ptr<Node>> &nodes,
                  const std::string &outputPath,
                  const std::unordered_set<std::string> &globals,
                  const std::unordered_map<std::string, std::vector<std::string>> &paramTypes,
                  const std::unordered_map<std::string, std::string> &globalTypes,
                  long seed);
};

} // namespace aym

#endif // AYM_LLVM_CODEGEN_H
