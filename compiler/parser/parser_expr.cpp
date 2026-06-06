#include "parser.h"
#include <memory>
#include <string>
#include <algorithm>
#include <vector>

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
                peek().type != TokenType::KeywordTypeString) {
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

std::unique_ptr<Expr> Parser::parseInterpolatedString(const Token &tok) {
    std::vector<std::unique_ptr<Expr>> parts;
    std::string literal;
    const std::string &text = tok.text;
    size_t i = 0;
    while (i < text.size()) {
        char ch = text[i];
        if (ch == '{') {
            size_t end = text.find('}', i + 1);
            if (end == std::string::npos) {
                parseError("interpolacion sin cierre '}'");
                return std::make_unique<StringExpr>(text);
            }
            if (!literal.empty()) {
                parts.push_back(std::make_unique<StringExpr>(literal));
                literal.clear();
            }
            std::string exprText = text.substr(i + 1, end - i - 1);
            Lexer lexer(exprText);
            auto exprTokens = lexer.tokenize();
            Parser exprParser(exprTokens);
            auto expr = exprParser.parseExpressionOnly();
            if (exprParser.hasError()) {
                parseError("error en interpolacion");
                return std::make_unique<StringExpr>(text);
            }
            parts.push_back(ensureString(std::move(expr)));
            i = end + 1;
            continue;
        }
        literal += ch;
        ++i;
    }
    if (!literal.empty()) {
        parts.push_back(std::make_unique<StringExpr>(literal));
    }
    return chainConcat(std::move(parts));
}

std::unique_ptr<Expr> Parser::parseListLiteral(const Token &tok) {
    std::vector<std::unique_ptr<Expr>> elements;
    if (peek().type != TokenType::RBracket) {
        elements.push_back(parseExpression());
        while (match(TokenType::Comma)) {
            elements.push_back(parseExpression());
        }
    }
    if (!match(TokenType::RBracket)) {
        parseError("se esperaba ']' en lista");
    }
    auto node = std::make_unique<ListExpr>(std::move(elements));
    node->setLocation(tok.line, tok.column);
    return node;
}

std::unique_ptr<Expr> Parser::parseMapLiteral(const Token &tok) {
    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> items;
    if (peek().type != TokenType::RBrace) {
        while (true) {
            auto key = parseExpression();
            if (!match(TokenType::Colon)) {
                parseError("se esperaba ':' en mapa");
            }
            auto value = parseExpression();
            items.emplace_back(std::move(key), std::move(value));
            if (!match(TokenType::Comma)) break;
        }
    }
    if (!match(TokenType::RBrace)) {
        parseError("se esperaba '}' en mapa");
    }
    auto node = std::make_unique<MapExpr>(std::move(items));
    node->setLocation(tok.line, tok.column);
    return node;
}

std::unique_ptr<Expr> Parser::parseFormatExpression(const Token &tok,
                                                    const std::string &format,
                                                    std::vector<std::unique_ptr<Expr>> args,
                                                    bool percentStyle) {
    std::vector<std::unique_ptr<Expr>> parts;
    size_t idx = 0;
    if (percentStyle) {
        std::string literal;
        for (size_t i = 0; i < format.size(); ++i) {
            if (format[i] == '%' && i + 1 < format.size()) {
                char spec = format[i + 1];
                if (spec == '%') {
                    literal += '%';
                    ++i;
                    continue;
                }
                if (spec == 's' || spec == 'd') {
                    if (!literal.empty()) {
                        parts.push_back(std::make_unique<StringExpr>(literal));
                        literal.clear();
                    }
                    if (idx >= args.size()) {
                        parseError("faltan argumentos en formato");
                        break;
                    }
                    parts.push_back(ensureString(std::move(args[idx++])));
                    ++i;
                    continue;
                }
            }
            literal += format[i];
        }
        if (!literal.empty()) {
            parts.push_back(std::make_unique<StringExpr>(literal));
        }
    } else {
        std::string literal;
        for (size_t i = 0; i < format.size(); ++i) {
            if (format[i] == '{') {
                if (i + 1 < format.size() && format[i + 1] == '}') {
                    if (!literal.empty()) {
                        parts.push_back(std::make_unique<StringExpr>(literal));
                        literal.clear();
                    }
                    if (idx >= args.size()) {
                        parseError("faltan argumentos en formato");
                        break;
                    }
                    parts.push_back(ensureString(std::move(args[idx++])));
                    ++i;
                    continue;
                }
            }
            literal += format[i];
        }
        if (!literal.empty()) {
            parts.push_back(std::make_unique<StringExpr>(literal));
        }
    }
    if (idx < args.size()) {
        parseError("sobran argumentos en formato");
    }
    auto node = chainConcat(std::move(parts));
    if (node) node->setLocation(tok.line, tok.column);
    return node;
}

std::unique_ptr<Expr> Parser::ensureString(std::unique_ptr<Expr> expr) {
    if (!expr) return expr;
    if (dynamic_cast<StringExpr*>(expr.get())) return expr;
    std::vector<std::unique_ptr<Expr>> args;
    args.push_back(std::move(expr));
    return std::make_unique<CallExpr>("aru", std::move(args));
}

std::unique_ptr<Expr> Parser::chainConcat(std::vector<std::unique_ptr<Expr>> parts) {
    if (parts.empty()) {
        return std::make_unique<StringExpr>("");
    }
    std::unique_ptr<Expr> current = std::move(parts[0]);
    for (size_t i = 1; i < parts.size(); ++i) {
        current = std::make_unique<BinaryExpr>('+', std::move(current), std::move(parts[i]));
    }
    return current;
}

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
