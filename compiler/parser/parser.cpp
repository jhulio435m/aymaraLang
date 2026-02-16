#include "parser.h"
#include <memory>
#include <string>
#include "../utils/diagnostic.h"

namespace aym {

namespace {
std::string canonicalTypeName(const Token &tok) {
    switch (tok.type) {
        case TokenType::KeywordTypeNumber:
            return "jakhüwi";
        case TokenType::KeywordTypeString:
            return "aru";
        case TokenType::KeywordTypeBool:
        case TokenType::KeywordTrue:
            return "chiqa";
        case TokenType::KeywordTypeList:
            return "t'aqa";
        case TokenType::KeywordTypeMap:
            return "mapa";
        default:
            return tok.text;
    }
}
} // namespace

Parser::Parser(const std::vector<Token>& t, DiagnosticEngine *diag)
    : tokens(t), diagnostics(diag) {}

std::string Parser::parseTypeName() {
    if (match(TokenType::KeywordTypeNumber) || match(TokenType::KeywordTypeString) ||
        match(TokenType::KeywordTypeBool) || match(TokenType::KeywordTypeList) ||
        match(TokenType::KeywordTypeMap) || match(TokenType::KeywordTrue)) {
        return canonicalTypeName(tokens[pos-1]);
    }
    if (match(TokenType::Identifier)) {
        return tokens[pos-1].text;
    }
    return "";
}

std::vector<std::unique_ptr<Node>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> stmts;
    bool hasStart = match(TokenType::KeywordStart);
    parseStatements(stmts);
    if (match(TokenType::KeywordEnd)) {
        if (peek().type != TokenType::EndOfFile) {
            parseError("token inesperado despues de 'tukuya'");
        }
    } else if (hasStart) {
        parseError("se esperaba 'tukuya' al final del programa");
    }
    std::vector<std::unique_ptr<Node>> nodes;
    for (auto &s : stmts) nodes.push_back(std::move(s));
    return nodes;
}

std::unique_ptr<Expr> Parser::parseExpressionOnly() {
    auto expr = parseExpression();
    if (!expr) {
        match(TokenType::Semicolon);
        auto node = std::make_unique<NumberExpr>(0);
        node->setLocation(tokens[pos-1].line, tokens[pos-1].column);
        return node;
    }
    if (peek().type != TokenType::EndOfFile) {
        parseError("token inesperado en expresion");
    }
    return expr;
}

const Token &Parser::peek() const { return tokens[pos]; }
const Token &Parser::get() { return tokens[pos++]; }

bool Parser::match(TokenType type) {
    if (peek().type == type) { get(); return true; }
    return false;
}

void Parser::parseError(const std::string &msg) {
    const Token &tok = peek();
    if (diagnostics) {
        diagnostics->error("AYM2001", msg, tok.line, tok.column);
    } else {
        std::cerr << "[parser] Error en linea " << tok.line << ", columna " << tok.column
                  << ": " << msg << std::endl;
    }
    hadError = true;
    synchronize();
}

void Parser::synchronize() {
    while (pos < tokens.size()) {
        TokenType t = peek().type;
        if (t == TokenType::Semicolon) { get(); break; }
        if (t == TokenType::RBrace || t == TokenType::EndOfFile) break;
        get();
    }
}

void Parser::parseStatements(std::vector<std::unique_ptr<Stmt>> &nodes, bool stopAtBrace) {
    while (pos < tokens.size() && peek().type != TokenType::EndOfFile) {
        if (peek().type == TokenType::KeywordEnd) { break; }
        if (stopAtBrace && peek().type == TokenType::RBrace) { get(); break; }
        size_t startPos = pos;
        auto stmt = parseSingleStatement();
        if (stmt) {
            nodes.push_back(std::move(stmt));
        }
        if (pos == startPos) {
            const Token &tok = peek();
            if (diagnostics) {
                diagnostics->error("AYM2002",
                                   "recuperacion forzada para evitar bucle infinito",
                                   tok.line, tok.column);
            } else {
                std::cerr << "[parser] Error en linea " << tok.line << ", columna " << tok.column
                          << ": recuperacion forzada para evitar bucle infinito" << std::endl;
            }
            hadError = true;
            if (tok.type == TokenType::EndOfFile) {
                break;
            }
            get();
        }
    }
}


} // namespace aym
