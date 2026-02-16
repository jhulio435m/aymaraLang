#include "parser.h"
#include <memory>

namespace aym {

std::unique_ptr<Stmt> Parser::parseSingleStatement() {
    if (match(TokenType::KeywordClass)) {
        return parseClassStatement();
    }
    if (match(TokenType::KeywordImport)) {
        return parseImportStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordEnum)) {
        return parseEnumStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordMatch)) {
        return parseMatchStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordThrow)) {
        return parseThrowStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordTry)) {
        return parseTryStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordDeclare)) {
        return parseVarDeclStatement(tokens[pos-1]);
    }

    if (match(TokenType::KeywordReturn)) {
        Token tok = tokens[pos-1];
        std::unique_ptr<Expr> val;
        if (peek().type != TokenType::Semicolon) val = parseExpression();
        match(TokenType::Semicolon);
        auto node = std::make_unique<ReturnStmt>(std::move(val));
        node->setLocation(tok.line, tok.column);
        return node;
    }

    if (match(TokenType::KeywordBreak)) {
        Token tok = tokens[pos-1];
        match(TokenType::Semicolon);
        auto node = std::make_unique<BreakStmt>();
        node->setLocation(tok.line, tok.column);
        return node;
    }

    if (match(TokenType::KeywordContinue)) {
        Token tok = tokens[pos-1];
        match(TokenType::Semicolon);
        auto node = std::make_unique<ContinueStmt>();
        node->setLocation(tok.line, tok.column);
        return node;
    }

    if (match(TokenType::KeywordPrint)) {
        return parsePrintStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordIf)) {
        return parseIfStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordWhile)) {
        return parseWhileStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordFor)) {
        return parseForStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordFunc)) {
        return parseFunctionStatement(tokens[pos-1]);
    }

    if (match(TokenType::Semicolon)) {
        auto node = std::make_unique<ExprStmt>(nullptr);
        node->setLocation(tokens[pos-1].line, tokens[pos-1].column);
        return node;
    }

    return parseExpressionOrAssignmentStatement();
}

} // namespace aym
