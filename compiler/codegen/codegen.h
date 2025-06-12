#ifndef AYM_CODEGEN_H
#define AYM_CODEGEN_H

#include <memory>
#include <string>
#include <vector>

namespace aym {

class Node;

class CodeGenerator {
public:
    void generate(const std::vector<std::unique_ptr<Node>> &nodes,
                  const std::string &outputPath);
};

} // namespace aym

#endif // AYM_CODEGEN_H
