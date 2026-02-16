#include "parser.h"
#include <memory>
#include <string>

namespace aym {

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

} // namespace aym
