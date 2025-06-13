#ifndef AYM_PARSER_H
#define AYM_PARSER_H

#include <vector>
#include <iostream>
#include <memory>
#include "../lexer/lexer.h"
#include "../ast/ast.h"

namespace aym {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::vector<std::unique_ptr<Node>> parse();
private:
    std::vector<Token> tokens;
    size_t pos = 0;

    const Token &peek() const;
    const Token &get();
    bool match(TokenType type);
    std::unique_ptr<Node> parseStatement();
    int parseExpression();
    int parseTerm();
    int parseFactor();
};

} // namespace aym

#endif // AYM_PARSER_H
