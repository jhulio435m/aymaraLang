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
        std::string text;
        if (peek().type == TokenType::String) {
            text = get().text;
        } else {
            int value = parseExpression();
            text = std::to_string(value);
        }
        if (!match(TokenType::RParen)) return nullptr;
        match(TokenType::Semicolon);
        return std::make_unique<PrintNode>(text);
    }
    return nullptr;
}

int Parser::parseExpression() {
    int value = parseTerm();
    while (true) {
        if (match(TokenType::Plus)) {
            value += parseTerm();
        } else if (match(TokenType::Minus)) {
            value -= parseTerm();
        } else {
            break;
        }
    }
    return value;
}

int Parser::parseTerm() {
    int value = parseFactor();
    while (true) {
        if (match(TokenType::Star)) {
            value *= parseFactor();
        } else if (match(TokenType::Slash)) {
            int divisor = parseFactor();
            if (divisor != 0) value /= divisor;
        } else {
            break;
        }
    }
    return value;
}

int Parser::parseFactor() {
    if (match(TokenType::Number)) {
        return std::stoi(tokens[pos-1].text);
    }
    if (match(TokenType::LParen)) {
        int value = parseExpression();
        match(TokenType::RParen);
        return value;
    }
    return 0;
}

} // namespace aym
