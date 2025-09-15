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

void Parser::parseError(const std::string &msg) {
    const Token &tok = peek();
    std::cerr << "[parser] Error en linea " << tok.line << ", columna " << tok.column
              << ": " << msg << std::endl;
    hadError = true;
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
        Token typeTok = tokens[pos-1];
        std::string type = typeTok.text;
        std::string name;
        if (peek().type == TokenType::Identifier) {
            name = get().text;
        }
        std::unique_ptr<Expr> init;
        if (match(TokenType::Equal)) init = parseExpression();
        match(TokenType::Semicolon);
        auto node = std::make_unique<VarDeclStmt>(type, name, std::move(init));
        node->setLocation(typeTok.line, typeTok.column);
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
        match(TokenType::LParen);
        auto expr = parseExpression();
        match(TokenType::RParen);
        match(TokenType::Semicolon);
        auto node = std::make_unique<PrintStmt>(std::move(expr));
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
            if (match(TokenType::KeywordIf)) {
                // else if -> treat as block containing nested if
                --pos; // unread 'if' so parseSingleStatement sees it
                auto nested = parseSingleStatement();
                elseBlock = std::make_unique<BlockStmt>();
                elseBlock->setLocation(elseTok.line, elseTok.column);
                elseBlock->statements.push_back(std::move(nested));
            } else {
                match(TokenType::LBrace);
                Token elseBrace = tokens[pos-1];
                elseBlock = std::make_unique<BlockStmt>();
                elseBlock->setLocation(elseBrace.line, elseBrace.column);
                parseStatements(elseBlock->statements, true);
            }
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

    if (match(TokenType::KeywordDo)) {
        Token doTok = tokens[pos-1];
        match(TokenType::LBrace);
        Token braceTok = tokens[pos-1];
        auto body = std::make_unique<BlockStmt>();
        body->setLocation(braceTok.line, braceTok.column);
        parseStatements(body->statements, true);
        match(TokenType::KeywordWhile);
        match(TokenType::LParen);
        auto cond = parseExpression();
        match(TokenType::RParen);
        match(TokenType::Semicolon);
        auto node = std::make_unique<DoWhileStmt>(std::move(body), std::move(cond));
        node->setLocation(doTok.line, doTok.column);
        return node;
    }

    if (match(TokenType::KeywordFor)) {
        Token forTok = tokens[pos-1];
        if (peek().type == TokenType::Identifier &&
            tokens[pos+1].type == TokenType::KeywordIn) {
            std::string name = get().text; // variable
            Token nameTok = tokens[pos-1];
            match(TokenType::KeywordIn);
            if (peek().type == TokenType::Identifier && peek().text == "range") {
                get();
                match(TokenType::LParen);
                auto start = parseExpression();
                match(TokenType::Comma);
                auto end = parseExpression();
                match(TokenType::RParen);
                match(TokenType::LBrace);
                Token braceTok = tokens[pos-1];
                auto body = std::make_unique<BlockStmt>();
                body->setLocation(braceTok.line, braceTok.column);
                parseStatements(body->statements, true);
                auto init = std::make_unique<VarDeclStmt>("jach’a", name, std::move(start));
                init->setLocation(nameTok.line, nameTok.column);
                auto cond = std::make_unique<BinaryExpr>('<', std::make_unique<VariableExpr>(name), std::move(end));
                cond->setLocation(nameTok.line, nameTok.column);
                auto postExpr = std::make_unique<BinaryExpr>('+', std::make_unique<VariableExpr>(name), std::make_unique<NumberExpr>(1));
                postExpr->setLocation(nameTok.line, nameTok.column);
                auto post = std::make_unique<AssignStmt>(name, std::move(postExpr));
                post->setLocation(nameTok.line, nameTok.column);
                auto node = std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(post), std::move(body));
                node->setLocation(forTok.line, forTok.column);
                return node;
            }
        }

        match(TokenType::LParen);
        auto init = parseSingleStatement();
        auto cond = parseExpression();
        match(TokenType::Semicolon);
        auto post = parseSingleStatement();
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

    if (match(TokenType::KeywordSwitch)) {
        Token switchTok = tokens[pos-1];
        match(TokenType::LParen);
        auto val = parseExpression();
        match(TokenType::RParen);
        match(TokenType::LBrace);
        std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> cases;
        std::unique_ptr<BlockStmt> defCase;
        while (peek().type != TokenType::RBrace && peek().type != TokenType::EndOfFile) {
            if (match(TokenType::KeywordCase)) {
                Token caseTok = tokens[pos-1];
                auto cval = parseExpression();
                match(TokenType::Colon);
                auto blk = std::make_unique<BlockStmt>();
                blk->setLocation(caseTok.line, caseTok.column);
                while (peek().type != TokenType::KeywordCase &&
                       peek().type != TokenType::KeywordDefault &&
                       peek().type != TokenType::RBrace &&
                       peek().type != TokenType::EndOfFile) {
                    blk->statements.push_back(parseSingleStatement());
                }
                cases.emplace_back(std::move(cval), std::move(blk));
            } else if (match(TokenType::KeywordDefault)) {
                Token defTok = tokens[pos-1];
                match(TokenType::Colon);
                defCase = std::make_unique<BlockStmt>();
                defCase->setLocation(defTok.line, defTok.column);
                while (peek().type != TokenType::RBrace &&
                       peek().type != TokenType::EndOfFile) {
                    defCase->statements.push_back(parseSingleStatement());
                }
            } else {
                get();
            }
        }
        match(TokenType::RBrace);
        auto node = std::make_unique<SwitchStmt>(std::move(val), std::move(cases), std::move(defCase));
        node->setLocation(switchTok.line, switchTok.column);
        return node;
    }

    if (match(TokenType::KeywordFunc)) {
        Token funcTok = tokens[pos-1];
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
        Token bodyTok = tokens[pos-1];
        auto body = std::make_unique<BlockStmt>();
        body->setLocation(bodyTok.line, bodyTok.column);
        parseStatements(body->statements, true);
        auto node = std::make_unique<FunctionStmt>(name, std::move(params), std::move(body));
        node->setLocation(funcTok.line, funcTok.column);
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
    return parseLogic();
}

std::unique_ptr<Expr> Parser::parseLogic() {
    auto lhs = parseEquality();
    while (true) {
        if (match(TokenType::KeywordAnd) || match(TokenType::AmpAmp)) {
            Token op = tokens[pos-1];
            auto rhs = parseEquality();
            auto node = std::make_unique<BinaryExpr>('&', std::move(lhs), std::move(rhs));
            node->setLocation(op.line, op.column);
            lhs = std::move(node);
        } else if (match(TokenType::KeywordOr) || match(TokenType::PipePipe)) {
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
    if (match(TokenType::KeywordNot) || match(TokenType::Bang)) {
        Token tok = tokens[pos-1];
        auto e = parseFactor();
        auto node = std::make_unique<UnaryExpr>('!', std::move(e));
        node->setLocation(tok.line, tok.column);
        return node;
    }
    if (match(TokenType::Number)) {
        Token tok = tokens[pos-1];
        auto node = std::make_unique<NumberExpr>(std::stol(tok.text));
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
        return node;
    }
    if (match(TokenType::LParen)) {
        auto expr = parseExpression();
        match(TokenType::RParen);
        return expr;
    }
    parseError("token inesperado");
    get();
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
