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
    std::unique_ptr<Expr> parseExpressionOnly();
    bool hasError() const { return hadError; }
private:
    std::vector<Token> tokens;
    size_t pos = 0;
    std::unordered_map<std::string, int> dummy; // reserved for future use
    bool hadError = false;

    const Token &peek() const;
    const Token &get();
    bool match(TokenType type);
    void parseError(const std::string &msg);
    void synchronize();
    void parseStatements(std::vector<std::unique_ptr<Stmt>> &nodes, bool stopAtBrace = false);
    std::unique_ptr<Stmt> parseSingleStatement();
    std::unique_ptr<Stmt> parseClassStatement();
    std::unique_ptr<ClassStmt> parseClassBody(const std::string &name, const std::string &baseName, const Token &startTok);
    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parsePrimary();
    std::unique_ptr<Expr> parseClassPrimary();
    std::string parseTypeName();
    std::unique_ptr<Expr> parseTernary();
    std::unique_ptr<Expr> parseLogic();
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseAdd();
    std::unique_ptr<Expr> parseTerm();
    std::unique_ptr<Expr> parsePower();
    std::unique_ptr<Expr> parseFactor();
    std::vector<std::unique_ptr<Expr>> parseArguments();
    std::unique_ptr<Expr> parseInterpolatedString(const Token &tok);
    std::unique_ptr<Expr> parseListLiteral(const Token &tok);
    std::unique_ptr<Expr> parseMapLiteral(const Token &tok);
    std::unique_ptr<Expr> parseFormatExpression(const Token &tok,
                                                const std::string &format,
                                                std::vector<std::unique_ptr<Expr>> args,
                                                bool percentStyle);
    std::unique_ptr<Expr> ensureString(std::unique_ptr<Expr> expr);
    std::unique_ptr<Expr> chainConcat(std::vector<std::unique_ptr<Expr>> parts);
};

} // namespace aym

#endif // AYM_PARSER_H
