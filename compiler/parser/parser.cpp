#include "parser.h"
#include <memory>

namespace aym {

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

std::vector<std::unique_ptr<Node>> Parser::parse() {
    std::vector<std::unique_ptr<Node>> nodes;
    while (!match(TokenType::EndOfFile)) {
        if (auto node = parseStatement()) {
            nodes.push_back(std::move(node));
        } else {
            get();
        }
    }
    return nodes;
}

const Token &Parser::peek() const {
    return tokens[pos];
}

const Token &Parser::get() {
    return tokens[pos++];
}

bool Parser::match(TokenType type) {
    if (peek().type == type) {
        get();
        return true;
    }
    return false;
}

std::unique_ptr<Node> Parser::parseStatement() {
    if (match(TokenType::KeywordPrint)) {
        if (!match(TokenType::LParen)) return nullptr;
        if (peek().type != TokenType::String) return nullptr;
        std::string text = get().text;
        if (!match(TokenType::RParen)) return nullptr;
        match(TokenType::Semicolon);
        return std::make_unique<PrintNode>(text);
    }
    return nullptr;
}

} // namespace aym
