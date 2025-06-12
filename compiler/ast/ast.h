#ifndef AYM_AST_H
#define AYM_AST_H

#include <string>

namespace aym {

class Node {
public:
    virtual ~Node() = default;
};

class PrintNode : public Node {
public:
    explicit PrintNode(const std::string &text) : text(text) {}
    const std::string &getText() const { return text; }
private:
    std::string text;
};

} // namespace aym

#endif // AYM_AST_H
