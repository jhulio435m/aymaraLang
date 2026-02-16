#include "parser.h"
#include <memory>

namespace aym {

std::unique_ptr<Stmt> Parser::parseVarDeclStatement(const Token &declTok) {
    std::string type = parseTypeName();
    if (type.empty()) {
        parseError("se esperaba un tipo despues de 'yatiya'");
    }
    std::string name;
    if (peek().type == TokenType::Identifier) {
        name = get().text;
    } else {
        parseError("se esperaba un identificador despues del tipo");
    }
    std::unique_ptr<Expr> init;
    if (match(TokenType::Equal)) init = parseExpression();
    if (!match(TokenType::Semicolon)) {
        parseError("se esperaba ';' despues de la declaracion");
    }
    auto node = std::make_unique<VarDeclStmt>(type, name, std::move(init));
    node->setLocation(declTok.line, declTok.column);
    return node;
}

std::unique_ptr<Stmt> Parser::parsePrintStatement(const Token &printTok) {
    std::vector<std::unique_ptr<Expr>> exprs;
    std::unique_ptr<Expr> separator;
    std::unique_ptr<Expr> terminator;
    match(TokenType::LParen);
    if (peek().type != TokenType::RParen) {
        while (true) {
            if ((peek().type == TokenType::Identifier || peek().type == TokenType::KeywordTypeList) &&
                pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::Equal) {
                std::string name = get().text;
                match(TokenType::Equal);
                auto value = parseExpression();
                if (name == "t'aqa") {
                    separator = std::move(value);
                } else if (name == "tuku") {
                    terminator = std::move(value);
                } else {
                    parseError("argumento nombrado desconocido en qillqa");
                }
            } else {
                exprs.push_back(parseExpression());
            }
            if (!match(TokenType::Comma)) {
                break;
            }
        }
    }
    match(TokenType::RParen);
    match(TokenType::Semicolon);
    auto node = std::make_unique<PrintStmt>(std::move(exprs), std::move(separator), std::move(terminator));
    node->setLocation(printTok.line, printTok.column);
    return node;
}

std::unique_ptr<Stmt> Parser::parseIfStatement(const Token &ifTok) {
    if (!match(TokenType::LParen)) {
        parseError("se esperaba '(' despues de 'jisa'");
    }
    auto cond = parseExpression();
    if (!match(TokenType::RParen)) {
        parseError("se esperaba ')' despues de la condicion de 'jisa'");
    }
    if (!match(TokenType::LBrace)) {
        parseError("se esperaba '{' en bloque de 'jisa'");
    }
    Token thenTok = tokens[pos > 0 ? pos - 1 : pos];
    auto thenBlock = std::make_unique<BlockStmt>();
    thenBlock->setLocation(thenTok.line, thenTok.column);
    parseStatements(thenBlock->statements, true);
    std::unique_ptr<BlockStmt> elseBlock;
    if (match(TokenType::KeywordElse)) {
        elseBlock = std::make_unique<BlockStmt>();
        Token elseTok = tokens[pos - 1];
        elseBlock->setLocation(elseTok.line, elseTok.column);
        if (match(TokenType::KeywordIf)) {
            Token nestedIfTok = tokens[pos - 1];
            elseBlock->statements.push_back(parseIfStatement(nestedIfTok));
        } else if (match(TokenType::LBrace)) {
            Token elseBrace = tokens[pos - 1];
            elseBlock->setLocation(elseBrace.line, elseBrace.column);
            parseStatements(elseBlock->statements, true);
        } else {
            parseError("se esperaba '{' o 'jisa' despues de 'maysatxa'");
        }
    }
    auto node = std::make_unique<IfStmt>(std::move(cond), std::move(thenBlock), std::move(elseBlock));
    node->setLocation(ifTok.line, ifTok.column);
    return node;
}

std::unique_ptr<Stmt> Parser::parseWhileStatement(const Token &whileTok) {
    if (!match(TokenType::LParen)) {
        parseError("se esperaba '(' despues de 'ukhakamaxa'");
    }
    auto cond = parseExpression();
    if (!match(TokenType::RParen)) {
        parseError("se esperaba ')' despues de la condicion de 'ukhakamaxa'");
    }
    Token braceTok = whileTok;
    if (match(TokenType::LBrace)) {
        braceTok = tokens[pos - 1];
    } else {
        parseError("se esperaba '{' en bloque de 'ukhakamaxa'");
    }
    auto block = std::make_unique<BlockStmt>();
    block->setLocation(braceTok.line, braceTok.column);
    parseStatements(block->statements, true);
    auto node = std::make_unique<WhileStmt>(std::move(cond), std::move(block));
    node->setLocation(whileTok.line, whileTok.column);
    return node;
}

} // namespace aym
