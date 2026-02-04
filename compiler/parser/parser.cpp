#include "parser.h"
#include <memory>
#include <string>
#include <algorithm>

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

std::string normalizeTypeName(const Token &tok) {
    std::string value = tok.text;
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}
} // namespace

Parser::Parser(const std::vector<Token>& t) : tokens(t) {}

std::vector<std::unique_ptr<Node>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> stmts;
    if (!match(TokenType::KeywordStart)) {
        parseError("se esperaba 'qallta' al inicio del programa");
    }
    parseStatements(stmts);
    if (!match(TokenType::KeywordEnd)) {
        parseError("se esperaba 'tukuya' al final del programa");
    }
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

void Parser::parseError(const std::string &msg) {
    const Token &tok = peek();
    std::cerr << "[parser] Error en linea " << tok.line << ", columna " << tok.column
              << ": " << msg << std::endl;
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
        nodes.push_back(parseSingleStatement());
    }
}

std::unique_ptr<Stmt> Parser::parseSingleStatement() {
    if (match(TokenType::KeywordImport)) {
        Token importTok = tokens[pos-1];
        std::string moduleName;
        if (match(TokenType::LParen)) {
            if (peek().type == TokenType::String || peek().type == TokenType::Identifier) {
                moduleName = get().text;
            } else {
                parseError("se esperaba la ruta del modulo despues de 'apnaq'");
            }
            match(TokenType::RParen);
        } else {
            parseError("se esperaba '(' despues de 'apnaq'");
        }
        if (!match(TokenType::Semicolon)) {
            parseError("se esperaba ';' despues de la declaracion de modulo");
        }
        auto node = std::make_unique<ImportStmt>(moduleName);
        node->setLocation(importTok.line, importTok.column);
        return node;
    }

    if (match(TokenType::KeywordDeclare)) {
        Token declTok = tokens[pos-1];
        std::string type;
        if (match(TokenType::KeywordTypeNumber) || match(TokenType::KeywordTypeString) ||
            match(TokenType::KeywordTypeBool) || match(TokenType::KeywordTypeList) ||
            match(TokenType::KeywordTypeMap) || match(TokenType::KeywordTrue)) {
            type = normalizeTypeName(tokens[pos-1]);
        } else {
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

    if (match(TokenType::KeywordReturn)) {
        Token tok = tokens[pos-1];
        std::unique_ptr<Expr> val;
        if (peek().type != TokenType::Semicolon) val = parseExpression();
        match(TokenType::Semicolon);
        auto node = std::make_unique<ReturnStmt>(std::move(val));
        node->setLocation(tok.line, tok.column);
        return node;
    }

    if (match(TokenType::KeywordBreak)) {
        Token tok = tokens[pos-1];
        match(TokenType::Semicolon);
        auto node = std::make_unique<BreakStmt>();
        node->setLocation(tok.line, tok.column);
        return node;
    }

    if (match(TokenType::KeywordContinue)) {
        Token tok = tokens[pos-1];
        match(TokenType::Semicolon);
        auto node = std::make_unique<ContinueStmt>();
        node->setLocation(tok.line, tok.column);
        return node;
    }

    if (match(TokenType::KeywordPrint)) {
        Token tok = tokens[pos-1];
        std::vector<std::unique_ptr<Expr>> exprs;
        std::unique_ptr<Expr> separator;
        std::unique_ptr<Expr> terminator;
        match(TokenType::LParen);
        if (peek().type != TokenType::RParen) {
            while (true) {
                if (peek().type == TokenType::Identifier && pos + 1 < tokens.size() &&
                    tokens[pos + 1].type == TokenType::Equal) {
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
        node->setLocation(tok.line, tok.column);
        return node;
    }

    if (match(TokenType::KeywordIf)) {
        Token ifTok = tokens[pos-1];
        match(TokenType::LParen);
        auto cond = parseExpression();
        match(TokenType::RParen);
        match(TokenType::LBrace);
        Token thenTok = tokens[pos-1];
        auto thenBlock = std::make_unique<BlockStmt>();
        thenBlock->setLocation(thenTok.line, thenTok.column);
        parseStatements(thenBlock->statements, true);
        std::unique_ptr<BlockStmt> elseBlock;
        if (match(TokenType::KeywordElse)) {
            Token elseTok = tokens[pos-1];
            match(TokenType::LBrace);
            Token elseBrace = tokens[pos-1];
            elseBlock = std::make_unique<BlockStmt>();
            elseBlock->setLocation(elseBrace.line, elseBrace.column);
            parseStatements(elseBlock->statements, true);
        }
        auto node = std::make_unique<IfStmt>(std::move(cond), std::move(thenBlock), std::move(elseBlock));
        node->setLocation(ifTok.line, ifTok.column);
        return node;
    }


    if (match(TokenType::KeywordWhile)) {
        Token tok = tokens[pos-1];
        match(TokenType::LParen);
        auto cond = parseExpression();
        match(TokenType::RParen);
        match(TokenType::LBrace);
        Token braceTok = tokens[pos-1];
        auto block = std::make_unique<BlockStmt>();
        block->setLocation(braceTok.line, braceTok.column);
        parseStatements(block->statements, true);
        auto node = std::make_unique<WhileStmt>(std::move(cond), std::move(block));
        node->setLocation(tok.line, tok.column);
        return node;
    }

    if (match(TokenType::KeywordFor)) {
        Token forTok = tokens[pos-1];
        match(TokenType::LParen);
        std::unique_ptr<Stmt> init = nullptr;
        if (!match(TokenType::Semicolon)) {
            if (match(TokenType::KeywordDeclare)) {
                Token declTok = tokens[pos-1];
                std::string type;
                if (match(TokenType::KeywordTypeNumber) || match(TokenType::KeywordTypeString) ||
                    match(TokenType::KeywordTypeBool) || match(TokenType::KeywordTypeList) ||
                    match(TokenType::KeywordTypeMap) || match(TokenType::KeywordTrue)) {
                    type = normalizeTypeName(tokens[pos-1]);
                } else {
                    parseError("se esperaba un tipo en el encabezado de 'sapüru'");
                }
                std::string name;
                if (peek().type == TokenType::Identifier) {
                    name = get().text;
                } else {
                    parseError("se esperaba un identificador en el encabezado de 'sapüru'");
                }
                std::unique_ptr<Expr> initExpr;
                if (match(TokenType::Equal)) initExpr = parseExpression();
                init = std::make_unique<VarDeclStmt>(type, name, std::move(initExpr));
                init->setLocation(declTok.line, declTok.column);
            } else if (peek().type == TokenType::Identifier && tokens[pos+1].type == TokenType::Equal) {
                std::string name = get().text;
                Token nameTok = tokens[pos-1];
                match(TokenType::Equal);
                auto value = parseExpression();
                init = std::make_unique<AssignStmt>(name, std::move(value));
                init->setLocation(nameTok.line, nameTok.column);
            } else {
                auto expr = parseExpression();
                size_t line = expr ? expr->getLine() : 0;
                size_t column = expr ? expr->getColumn() : 0;
                init = std::make_unique<ExprStmt>(std::move(expr));
                init->setLocation(line, column);
            }
            match(TokenType::Semicolon);
        }
        if (!init) init = std::make_unique<ExprStmt>(nullptr);
        std::unique_ptr<Expr> cond;
        if (peek().type != TokenType::Semicolon) {
            cond = parseExpression();
        }
        match(TokenType::Semicolon);
        std::unique_ptr<Stmt> post = nullptr;
        if (peek().type != TokenType::RParen) {
            if (peek().type == TokenType::Identifier && tokens[pos+1].type == TokenType::Equal) {
                std::string name = get().text;
                Token nameTok = tokens[pos-1];
                match(TokenType::Equal);
                auto value = parseExpression();
                post = std::make_unique<AssignStmt>(name, std::move(value));
                post->setLocation(nameTok.line, nameTok.column);
            } else {
                auto expr = parseExpression();
                size_t line = expr ? expr->getLine() : 0;
                size_t column = expr ? expr->getColumn() : 0;
                post = std::make_unique<ExprStmt>(std::move(expr));
                post->setLocation(line, column);
            }
            match(TokenType::Semicolon);
        }
        if (!post) post = std::make_unique<ExprStmt>(nullptr);
        match(TokenType::RParen);
        match(TokenType::LBrace);
        Token braceTok = tokens[pos-1];
        auto body = std::make_unique<BlockStmt>();
        body->setLocation(braceTok.line, braceTok.column);
        parseStatements(body->statements, true);
        auto node = std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(post), std::move(body));
        node->setLocation(forTok.line, forTok.column);
        return node;
    }

    if (match(TokenType::KeywordFunc)) {
        Token funcTok = tokens[pos-1];
        std::string name = "";
        if (peek().type == TokenType::Identifier) name = get().text;
        match(TokenType::LParen);
        std::vector<FunctionStmt::Param> params;
        if (peek().type != TokenType::RParen) {
            FunctionStmt::Param param;
                if (match(TokenType::KeywordTypeNumber) || match(TokenType::KeywordTypeString) ||
                    match(TokenType::KeywordTypeBool) || match(TokenType::KeywordTypeList) ||
                    match(TokenType::KeywordTypeMap) || match(TokenType::KeywordTrue)) {
                    param.type = normalizeTypeName(tokens[pos-1]);
                } else {
                    parseError("se esperaba un tipo de parametro");
                }
            if (peek().type == TokenType::Identifier) {
                param.name = get().text;
            } else {
                parseError("se esperaba un nombre de parametro");
            }
            params.push_back(param);
            while (match(TokenType::Comma)) {
                FunctionStmt::Param next;
                if (match(TokenType::KeywordTypeNumber) || match(TokenType::KeywordTypeString) ||
                    match(TokenType::KeywordTypeBool) || match(TokenType::KeywordTypeList) ||
                    match(TokenType::KeywordTypeMap) || match(TokenType::KeywordTrue)) {
                    next.type = normalizeTypeName(tokens[pos-1]);
                } else {
                    parseError("se esperaba un tipo de parametro");
                }
                if (peek().type == TokenType::Identifier) {
                    next.name = get().text;
                } else {
                    parseError("se esperaba un nombre de parametro");
                }
                params.push_back(next);
            }
        }
        match(TokenType::RParen);
        std::string returnType;
        if (match(TokenType::Colon)) {
            if (match(TokenType::KeywordTypeNumber) || match(TokenType::KeywordTypeString) ||
                match(TokenType::KeywordTypeBool) || match(TokenType::KeywordTypeList) ||
                match(TokenType::KeywordTypeMap) || match(TokenType::KeywordTrue)) {
                returnType = normalizeTypeName(tokens[pos-1]);
            } else {
                parseError("se esperaba un tipo de retorno");
            }
        }
        match(TokenType::LBrace);
        Token bodyTok = tokens[pos-1];
        auto body = std::make_unique<BlockStmt>();
        body->setLocation(bodyTok.line, bodyTok.column);
        parseStatements(body->statements, true);
        auto node = std::make_unique<FunctionStmt>(name, std::move(params), std::move(returnType), std::move(body));
        node->setLocation(funcTok.line, funcTok.column);
        return node;
    }

    if (match(TokenType::Semicolon)) {
        auto node = std::make_unique<ExprStmt>(nullptr);
        node->setLocation(tokens[pos-1].line, tokens[pos-1].column);
        return node;
    }

    if (peek().type == TokenType::Identifier) {
        std::string name = get().text;
        Token nameTok = tokens[pos-1];
        if (match(TokenType::Equal)) {
            auto value = parseExpression();
            match(TokenType::Semicolon);
            auto node = std::make_unique<AssignStmt>(name, std::move(value));
            node->setLocation(nameTok.line, nameTok.column);
            return node;
        }
        // not an assignment, rewind so expression parsing sees the identifier
        --pos;
    }

    // Fallback: expression statement
    auto expr = parseExpression();
    size_t line = expr ? expr->getLine() : 0;
    size_t column = expr ? expr->getColumn() : 0;
    match(TokenType::Semicolon);
    auto node = std::make_unique<ExprStmt>(std::move(expr));
    node->setLocation(line, column);
    return node;
}

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
            auto rhs = parsePower();
            auto node = std::make_unique<BinaryExpr>('%', std::move(lhs), std::move(rhs));
            node->setLocation(op.line, op.column);
            lhs = std::move(node);
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
        if (match(TokenType::Identifier)) {
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
    if (match(TokenType::String)) {
        Token tok = tokens[pos-1];
        auto node = std::make_unique<StringExpr>(tok.text);
        node->setLocation(tok.line, tok.column);
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
        if (match(TokenType::PlusPlus) || match(TokenType::MinusMinus)) {
            Token opTok = tokens[pos-1];
            bool increment = (opTok.type == TokenType::PlusPlus);
            auto incNode = std::make_unique<IncDecExpr>(name, increment, false);
            incNode->setLocation(opTok.line, opTok.column);
            return incNode;
        }
        return node;
    }
    if (match(TokenType::LParen)) {
        auto expr = parseExpression();
        match(TokenType::RParen);
        return expr;
    }
    parseError("token inesperado");
    return nullptr;
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
