#ifndef AYM_AST_JSON_H
#define AYM_AST_JSON_H

#include <memory>
#include <string>
#include <vector>

namespace aym {

class Node;

bool writeAstJson(const std::vector<std::unique_ptr<Node>> &nodes,
                  const std::string &path,
                  std::string &error);

} // namespace aym

#endif // AYM_AST_JSON_H
