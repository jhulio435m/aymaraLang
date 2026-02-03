#ifndef AYM_CODEGEN_H
#define AYM_CODEGEN_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace aym {

class Node;

class CodeGenerator {
public:
    void generate(const std::vector<std::unique_ptr<Node>> &nodes,
                  const std::string &outputPath,
                  const std::unordered_set<std::string> &globals,
                  const std::unordered_map<std::string, std::vector<std::string>> &paramTypes,
                  const std::unordered_map<std::string, std::string> &globalTypes,
                  bool windows,
                  long seed,
                  const std::string &runtimeDir);
};

} // namespace aym

#endif // AYM_CODEGEN_H
