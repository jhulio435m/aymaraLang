#include "parser.h"
#include <memory>
#include <string>

namespace aym {

std::unique_ptr<Expr> Parser::parseExpression() {
    return parseTernary();
}

std::unique_ptr<Expr> Parser::parseTernary() {
    auto cond = parseLogic();
    if (match(TokenType::Question)) {
        Token tok = tokens[pos-1];
        auto thenExpr = parseExpression();
        if (!match(TokenType::Colon)) {
            parseError("se esperaba ':' en operador ternario");
        }
        auto elseExpr = parseTernary();
        auto node = std::make_unique<TernaryExpr>(std::move(cond), std::move(thenExpr), std::move(elseExpr));
        node->setLocation(tok.line, tok.column);
        return node;
    }
    return cond;
}

std::unique_ptr<Expr> Parser::parseLogic() {
    auto lhs = parseEquality();
    while (true) {
        if (match(TokenType::AmpAmp)) {
            Token op = tokens[pos-1];
            auto rhs = parseEquality();
            auto node = std::make_unique<BinaryExpr>('&', std::move(lhs), std::move(rhs));
            node->setLocation(op.line, op.column);
            lhs = std::move(node);
        } else if (match(TokenType::PipePipe)) {
            Token op = tokens[pos-1];
            auto rhs = parseEquality();
            auto node = std::make_unique<BinaryExpr>('|', std::move(lhs), std::move(rhs));
            node->setLocation(op.line, op.column);
            lhs = std::move(node);
        } else {
            break;
        }
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parseEquality() {
    auto lhs = parseComparison();
    while (true) {
        if (match(TokenType::EqualEqual)) {
            Token op = tokens[pos-1];
            auto rhs = parseComparison();
            auto node = std::make_unique<BinaryExpr>('s', std::move(lhs), std::move(rhs));
            node->setLocation(op.line, op.column);
            lhs = std::move(node);
        } else if (match(TokenType::BangEqual)) {
            Token op = tokens[pos-1];
            auto rhs = parseComparison();
            auto node = std::make_unique<BinaryExpr>('d', std::move(lhs), std::move(rhs));
            node->setLocation(op.line, op.column);
            lhs = std::move(node);
        } else {
            break;
        }
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parseComparison() {
    auto lhs = parseAdd();
    while (true) {
        if (match(TokenType::Less)) {
            Token op = tokens[pos-1];
            auto rhs = parseAdd();
            auto node = std::make_unique<BinaryExpr>('<', std::move(lhs), std::move(rhs));
            node->setLocation(op.line, op.column);
            lhs = std::move(node);
        } else if (match(TokenType::LessEqual)) {
            Token op = tokens[pos-1];
            auto rhs = parseAdd();
            auto node = std::make_unique<BinaryExpr>('l', std::move(lhs), std::move(rhs));
            node->setLocation(op.line, op.column);
            lhs = std::move(node);
        } else if (match(TokenType::Greater)) {
            Token op = tokens[pos-1];
            auto rhs = parseAdd();
            auto node = std::make_unique<BinaryExpr>('>', std::move(lhs), std::move(rhs));
            node->setLocation(op.line, op.column);
            lhs = std::move(node);
        } else if (match(TokenType::GreaterEqual)) {
            Token op = tokens[pos-1];
            auto rhs = parseAdd();
            auto node = std::make_unique<BinaryExpr>('g', std::move(lhs), std::move(rhs));
            node->setLocation(op.line, op.column);
            lhs = std::move(node);
        } else {
            break;
        }
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parseAdd() {
    auto lhs = parseTerm();
    while (true) {
        if (match(TokenType::Plus)) {
            Token op = tokens[pos-1];
            auto rhs = parseTerm();
            auto node = std::make_unique<BinaryExpr>('+', std::move(lhs), std::move(rhs));
            node->setLocation(op.line, op.column);
            lhs = std::move(node);
        } else if (match(TokenType::Minus)) {
            Token op = tokens[pos-1];
            auto rhs = parseTerm();
            auto node = std::make_unique<BinaryExpr>('-', std::move(lhs), std::move(rhs));
            node->setLocation(op.line, op.column);
            lhs = std::move(node);
        } else {
            break;
        }
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parseTerm() {
    auto lhs = parsePower();
    while (true) {
        if (match(TokenType::Star)) {
            Token op = tokens[pos-1];
            auto rhs = parsePower();
            auto node = std::make_unique<BinaryExpr>('*', std::move(lhs), std::move(rhs));
            node->setLocation(op.line, op.column);
            lhs = std::move(node);
        } else if (match(TokenType::Slash)) {
            Token op = tokens[pos-1];
            auto rhs = parsePower();
            auto node = std::make_unique<BinaryExpr>('/', std::move(lhs), std::move(rhs));
            node->setLocation(op.line, op.column);
            lhs = std::move(node);
        } else if (match(TokenType::Percent)) {
            Token op = tokens[pos-1];
            if (auto *fmt = dynamic_cast<StringExpr*>(lhs.get())) {
                std::vector<std::unique_ptr<Expr>> args;
                if (match(TokenType::LParen)) {
                    if (peek().type != TokenType::RParen) {
                        args.push_back(parseExpression());
                        while (match(TokenType::Comma)) {
                            args.push_back(parseExpression());
                        }
                    }
                    match(TokenType::RParen);
                } else {
                    args.push_back(parsePower());
                }
                lhs = parseFormatExpression(op, fmt->getValue(), std::move(args), true);
            } else {
                auto rhs = parsePower();
                auto node = std::make_unique<BinaryExpr>('%', std::move(lhs), std::move(rhs));
                node->setLocation(op.line, op.column);
                lhs = std::move(node);
            }
        } else {
            break;
        }
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parsePower() {
    auto lhs = parseFactor();
    while (match(TokenType::Caret)) {
        Token op = tokens[pos-1];
        auto rhs = parseFactor();
        auto node = std::make_unique<BinaryExpr>('^', std::move(lhs), std::move(rhs));
        node->setLocation(op.line, op.column);
        lhs = std::move(node);
    }
    return lhs;
}


} // namespace aym
