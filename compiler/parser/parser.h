#ifndef AYM_PARSER_H
#define AYM_PARSER_H

#include <vector>
#include <iostream>
#include <memory>
#include <unordered_map>
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
    std::unordered_map<std::string, int> dummy; // reserved for future use

    const Token &peek() const;
    const Token &get();
    bool match(TokenType type);
    void parseStatements(std::vector<std::unique_ptr<Stmt>> &nodes, bool stopAtBrace = false);
    std::unique_ptr<Stmt> parseSingleStatement();
    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseTerm();
    std::unique_ptr<Expr> parseFactor();
    std::vector<std::unique_ptr<Expr>> parseArguments();
};

} // namespace aym

#endif // AYM_PARSER_H
