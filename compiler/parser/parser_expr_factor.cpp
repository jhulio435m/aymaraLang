#include "parser.h"
#include <memory>
#include <string>

namespace aym {

std::unique_ptr<Expr> Parser::parseFactor() {
    if (match(TokenType::PlusPlus) || match(TokenType::MinusMinus)) {
        Token tok = tokens[pos-1];
        bool increment = (tok.type == TokenType::PlusPlus);
        if (match(TokenType::Identifier) || match(TokenType::KeywordThis)) {
            Token idTok = tokens[pos-1];
            auto node = std::make_unique<IncDecExpr>(idTok.text, increment, true);
            node->setLocation(tok.line, tok.column);
            return node;
        }
        parseError("se esperaba un identificador despues de incremento/decremento");
        return nullptr;
    }
    if (match(TokenType::Minus)) {
        Token tok = tokens[pos-1];
        auto e = parseFactor();
        auto node = std::make_unique<UnaryExpr>('-', std::move(e));
        node->setLocation(tok.line, tok.column);
        return node;
    }
    if (match(TokenType::Plus)) {
        Token tok = tokens[pos-1];
        auto e = parseFactor();
        auto node = std::make_unique<UnaryExpr>('+', std::move(e));
        node->setLocation(tok.line, tok.column);
        return node;
    }
    if (match(TokenType::Bang)) {
        Token tok = tokens[pos-1];
        auto e = parseFactor();
        auto node = std::make_unique<UnaryExpr>('!', std::move(e));
        node->setLocation(tok.line, tok.column);
        return node;
    }
    auto node = parsePrimary();
    if (!node) return nullptr;
    while (true) {
        if (match(TokenType::LBracket)) {
            Token bracketTok = tokens[pos-1];
            auto index = parseExpression();
            if (!match(TokenType::RBracket)) {
                parseError("se esperaba ']' en acceso de lista");
            }
            auto indexed = std::make_unique<IndexExpr>(std::move(node), std::move(index));
            indexed->setLocation(bracketTok.line, bracketTok.column);
            node = std::move(indexed);
            continue;
        }
        if (peek().type == TokenType::Dot) {
            // '..' is reserved for ranges inside match cases.
            if ((pos + 1) < tokens.size() && tokens[pos + 1].type == TokenType::Dot) {
                break;
            }
            get();
            Token dotTok = tokens[pos-1];
            if (peek().type != TokenType::Identifier &&
                peek().type != TokenType::KeywordIf &&
                peek().type != TokenType::KeywordTypeString &&
                peek().type != TokenType::KeywordGetter &&
                peek().type != TokenType::KeywordSetter) {
                parseError("se esperaba nombre de miembro despues de '.'");
                break;
            }
            std::string member = get().text;
            if (match(TokenType::LParen)) {
                auto args = parseArguments();
                match(TokenType::RParen);
                auto callNode = std::make_unique<MemberCallExpr>(std::move(node), member, std::move(args));
                callNode->setLocation(dotTok.line, dotTok.column);
                node = std::move(callNode);
            } else {
                auto memberNode = std::make_unique<MemberExpr>(std::move(node), member);
                memberNode->setLocation(dotTok.line, dotTok.column);
                node = std::move(memberNode);
            }
            continue;
        }
        if (match(TokenType::PlusPlus) || match(TokenType::MinusMinus)) {
            Token opTok = tokens[pos-1];
            bool increment = (opTok.type == TokenType::PlusPlus);
            if (auto *var = dynamic_cast<VariableExpr*>(node.get())) {
                auto incNode = std::make_unique<IncDecExpr>(var->getName(), increment, false);
                incNode->setLocation(opTok.line, opTok.column);
                return incNode;
            }
            parseError("incremento/decremento solo permitido en variables");
            return node;
        }
        break;
    }
    return node;
}


std::vector<std::unique_ptr<Expr>> Parser::parseArguments() {
    std::vector<std::unique_ptr<Expr>> args;
    if (peek().type == TokenType::RParen) return args;
    if (peek().type == TokenType::Identifier && pos + 1 < tokens.size() &&
        tokens[pos + 1].type == TokenType::Equal) {
        get();
        match(TokenType::Equal);
        args.push_back(parseExpression());
    } else {
        args.push_back(parseExpression());
    }
    while (match(TokenType::Comma)) {
        if (peek().type == TokenType::Identifier && pos + 1 < tokens.size() &&
            tokens[pos + 1].type == TokenType::Equal) {
            get();
            match(TokenType::Equal);
            args.push_back(parseExpression());
        } else {
            args.push_back(parseExpression());
        }
    }
    return args;
}



} // namespace aym
