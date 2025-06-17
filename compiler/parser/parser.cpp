#include "parser.h"
#include <memory>

namespace aym {

Parser::Parser(const std::vector<Token>& t) : tokens(t) {}

std::vector<std::unique_ptr<Node>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> stmts;
    parseStatements(stmts);
    std::vector<std::unique_ptr<Node>> nodes;
    for (auto &s : stmts) nodes.push_back(std::move(s));
    return nodes;
}

const Token &Parser::peek() const { return tokens[pos]; }
const Token &Parser::get() { return tokens[pos++]; }

bool Parser::match(TokenType type) {
    if (peek().type == type) { get(); return true; }
    return false;
}

void Parser::parseStatements(std::vector<std::unique_ptr<Stmt>> &nodes, bool stopAtBrace) {
    while (pos < tokens.size() && peek().type != TokenType::EndOfFile) {
        if (stopAtBrace && peek().type == TokenType::RBrace) { get(); break; }
        nodes.push_back(parseSingleStatement());
    }
}

std::unique_ptr<Stmt> Parser::parseSingleStatement() {
    if (match(TokenType::KeywordInt) || match(TokenType::KeywordFloat) ||
        match(TokenType::KeywordBool) || match(TokenType::KeywordString)) {
        std::string type = tokens[pos-1].text;
        std::string name;
        if (peek().type == TokenType::Identifier) name = get().text;
        std::unique_ptr<Expr> init;
        if (match(TokenType::Equal)) init = parseExpression();
        match(TokenType::Semicolon);
        return std::make_unique<VarDeclStmt>(type, name, std::move(init));
    }

    if (match(TokenType::KeywordReturn)) {
        std::unique_ptr<Expr> val;
        if (peek().type != TokenType::Semicolon) val = parseExpression();
        match(TokenType::Semicolon);
        return std::make_unique<ReturnStmt>(std::move(val));
    }

    if (match(TokenType::KeywordBreak)) {
        match(TokenType::Semicolon);
        return std::make_unique<BreakStmt>();
    }

    if (match(TokenType::KeywordContinue)) {
        match(TokenType::Semicolon);
        return std::make_unique<ContinueStmt>();
    }

    if (match(TokenType::KeywordPrint)) {
        match(TokenType::LParen);
        auto expr = parseExpression();
        match(TokenType::RParen);
        match(TokenType::Semicolon);
        return std::make_unique<PrintStmt>(std::move(expr));
    }

    if (match(TokenType::KeywordIf)) {
        match(TokenType::LParen);
        auto cond = parseExpression();
        match(TokenType::RParen);
        match(TokenType::LBrace);
        auto thenBlock = std::make_unique<BlockStmt>();
        parseStatements(thenBlock->statements, true);
        std::unique_ptr<BlockStmt> elseBlock;
        if (match(TokenType::KeywordElse)) {
            match(TokenType::LBrace);
            elseBlock = std::make_unique<BlockStmt>();
            parseStatements(elseBlock->statements, true);
        }
        return std::make_unique<IfStmt>(std::move(cond), std::move(thenBlock), std::move(elseBlock));
    }

    if (match(TokenType::KeywordWhile)) {
        match(TokenType::LParen);
        auto cond = parseExpression();
        match(TokenType::RParen);
        match(TokenType::LBrace);
        auto block = std::make_unique<BlockStmt>();
        parseStatements(block->statements, true);
        return std::make_unique<WhileStmt>(std::move(cond), std::move(block));
    }

    if (match(TokenType::KeywordFor)) {
        match(TokenType::LParen);
        auto init = parseSingleStatement();
        auto cond = parseExpression();
        match(TokenType::Semicolon);
        auto post = parseSingleStatement();
        match(TokenType::RParen);
        match(TokenType::LBrace);
        auto body = std::make_unique<BlockStmt>();
        parseStatements(body->statements, true);
        return std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(post), std::move(body));
    }

    if (match(TokenType::KeywordFunc)) {
        std::string name = "";
        if (peek().type == TokenType::Identifier) name = get().text;
        match(TokenType::LParen);
        std::vector<std::string> params;
        if (peek().type != TokenType::RParen) {
            params.push_back(get().text);
            while (match(TokenType::Comma)) params.push_back(get().text);
        }
        match(TokenType::RParen);
        match(TokenType::LBrace);
        auto body = std::make_unique<BlockStmt>();
        parseStatements(body->statements, true);
        return std::make_unique<FunctionStmt>(name, std::move(params), std::move(body));
    }

    if (peek().type == TokenType::Identifier) {
        std::string name = get().text;
        if (match(TokenType::Equal)) {
            auto value = parseExpression();
            match(TokenType::Semicolon);
            return std::make_unique<AssignStmt>(name, std::move(value));
        }
        // not an assignment, rewind so expression parsing sees the identifier
        --pos;
    }

    // Fallback: expression statement
    auto expr = parseExpression();
    match(TokenType::Semicolon);
    return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<Expr> Parser::parseExpression() {
    auto lhs = parseTerm();
    while (true) {
        if (match(TokenType::Plus)) {
            auto rhs = parseTerm();
            lhs = std::make_unique<BinaryExpr>('+', std::move(lhs), std::move(rhs));
        } else if (match(TokenType::Minus)) {
            auto rhs = parseTerm();
            lhs = std::make_unique<BinaryExpr>('-', std::move(lhs), std::move(rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parseTerm() {
    auto lhs = parseFactor();
    while (true) {
        if (match(TokenType::Star)) {
            auto rhs = parseFactor();
            lhs = std::make_unique<BinaryExpr>('*', std::move(lhs), std::move(rhs));
        } else if (match(TokenType::Slash)) {
            auto rhs = parseFactor();
            lhs = std::make_unique<BinaryExpr>('/', std::move(lhs), std::move(rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parseFactor() {
    if (match(TokenType::Number)) {
        return std::make_unique<NumberExpr>(std::stol(tokens[pos-1].text));
    }
    if (match(TokenType::String)) {
        return std::make_unique<StringExpr>(tokens[pos-1].text);
    }
    if (match(TokenType::Identifier)) {
        std::string name = tokens[pos-1].text;
        if (match(TokenType::LParen)) {
            auto args = parseArguments();
            match(TokenType::RParen);
            return std::make_unique<CallExpr>(name, std::move(args));
        }
        return std::make_unique<VariableExpr>(name);
    }
    if (match(TokenType::LParen)) {
        auto expr = parseExpression();
        match(TokenType::RParen);
        return expr;
    }
    return std::make_unique<NumberExpr>(0);
}

std::vector<std::unique_ptr<Expr>> Parser::parseArguments() {
    std::vector<std::unique_ptr<Expr>> args;
    if (peek().type == TokenType::RParen) return args;
    args.push_back(parseExpression());
    while (match(TokenType::Comma)) {
        args.push_back(parseExpression());
    }
    return args;
}

} // namespace aym
