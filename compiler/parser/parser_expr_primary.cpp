#include "parser.h"
#include <memory>
#include <string>

namespace aym {

namespace {
long long parseNumberLiteral(const Token &tok) {
    const std::string &text = tok.text;
    if (text.size() > 2 && (text[0] == '0') && (text[1] == 'b' || text[1] == 'B')) {
        long long value = 0;
        for (size_t i = 2; i < text.size(); ++i) {
            value = (value << 1) + (text[i] == '1' ? 1 : 0);
        }
        return value;
    }
    return std::stoll(text, nullptr, 0);
}
} // namespace

std::unique_ptr<Expr> Parser::parsePrimary() {
    if (match(TokenType::Amp)) {
        Token ampTok = tokens[pos-1];
        if (peek().type != TokenType::Identifier) {
            parseError("se esperaba identificador despues de '&'");
            return nullptr;
        }
        std::string fn = get().text;
        auto node = std::make_unique<FunctionRefExpr>(fn);
        node->setLocation(ampTok.line, ampTok.column);
        return node;
    }
    if (auto classExpr = parseClassPrimary()) {
        return classExpr;
    }
    if (match(TokenType::Number)) {
        Token tok = tokens[pos-1];
        auto node = std::make_unique<NumberExpr>(parseNumberLiteral(tok));
        node->setLocation(tok.line, tok.column);
        return node;
    }
    if (match(TokenType::KeywordTrue)) {
        Token tok = tokens[pos-1];
        auto node = std::make_unique<BoolExpr>(true);
        node->setLocation(tok.line, tok.column);
        return node;
    }
    if (match(TokenType::KeywordFalse)) {
        Token tok = tokens[pos-1];
        auto node = std::make_unique<BoolExpr>(false);
        node->setLocation(tok.line, tok.column);
        return node;
    }
    if (match(TokenType::InterpolatedString)) {
        Token tok = tokens[pos-1];
        auto node = parseInterpolatedString(tok);
        if (node) node->setLocation(tok.line, tok.column);
        return node;
    }
    if (match(TokenType::String)) {
        Token tok = tokens[pos-1];
        auto node = std::make_unique<StringExpr>(tok.text);
        node->setLocation(tok.line, tok.column);
        if (match(TokenType::Dot)) {
            if (peek().type != TokenType::Identifier) {
                parseError("se esperaba nombre de metodo despues de '.'");
                return node;
            }
            std::string method = get().text;
            if (method != "fmt") {
                parseError("metodo desconocido en texto");
                return node;
            }
            match(TokenType::LParen);
            std::vector<std::unique_ptr<Expr>> args;
            if (peek().type != TokenType::RParen) {
                args.push_back(parseExpression());
                while (match(TokenType::Comma)) {
                    args.push_back(parseExpression());
                }
            }
            match(TokenType::RParen);
            return parseFormatExpression(tok, tok.text, std::move(args), false);
        }
        return node;
    }
    if (match(TokenType::LBracket)) {
        Token tok = tokens[pos-1];
        return parseListLiteral(tok);
    }
    if (match(TokenType::LBrace)) {
        Token tok = tokens[pos-1];
        return parseMapLiteral(tok);
    }
    if (match(TokenType::KeywordTypeString) || match(TokenType::KeywordTypeNumber)) {
        Token idTok = tokens[pos-1];
        std::string name = idTok.text;
        if (match(TokenType::LParen)) {
            auto args = parseArguments();
            match(TokenType::RParen);
            auto node = std::make_unique<CallExpr>(name, std::move(args));
            node->setLocation(idTok.line, idTok.column);
            return node;
        }
        auto node = std::make_unique<VariableExpr>(name);
        node->setLocation(idTok.line, idTok.column);
        return node;
    }
    if (match(TokenType::Identifier)) {
        Token idTok = tokens[pos-1];
        std::string name = idTok.text;
        if (match(TokenType::LParen)) {
            auto args = parseArguments();
            match(TokenType::RParen);
            auto node = std::make_unique<CallExpr>(name, std::move(args));
            node->setLocation(idTok.line, idTok.column);
            return node;
        }
        auto node = std::make_unique<VariableExpr>(name);
        node->setLocation(idTok.line, idTok.column);
        return node;
    }
    if (match(TokenType::LParen)) {
        std::vector<std::unique_ptr<Expr>> exprs;
        if (peek().type != TokenType::RParen) {
            exprs.push_back(parseExpression());
            while (match(TokenType::Comma)) {
                exprs.push_back(parseExpression());
            }
        }
        match(TokenType::RParen);
        if (exprs.empty()) {
            parseError("expresion vacia");
            return nullptr;
        }
        if (exprs.size() == 1) {
            return std::move(exprs[0]);
        }
        parseError("se esperaba una sola expresion");
        return nullptr;
    }
    parseError("token inesperado");
    return nullptr;
}

} // namespace aym
