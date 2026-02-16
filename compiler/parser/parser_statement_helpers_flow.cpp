#include "parser.h"
#include <memory>
#include <string>
#include <algorithm>

namespace aym {

namespace {
std::string normalizeTypeNameLocal(const Token &tok) {
    std::string value = tok.text;
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}
} // namespace

std::unique_ptr<Stmt> Parser::parseForStatement(const Token &forTok) {
    if (!match(TokenType::LParen)) {
        parseError("se esperaba '(' despues de 'taki'");
    }
    // foreach desugaring: taki(yatiya tipo item: listaIdentificador) { ... }
    if (peek().type == TokenType::KeywordDeclare) {
        size_t startPos = pos;
        get(); // consume yatiya
        std::string itemType = parseTypeName();
        std::string itemName;
        if (peek().type == TokenType::Identifier) {
            itemName = get().text;
        } else {
            parseError("se esperaba identificador en foreach");
        }
        if (match(TokenType::Colon)) {
            auto iterableExpr = parseExpression();
            auto *iterVar = dynamic_cast<VariableExpr*>(iterableExpr.get());
            if (!iterVar) {
                parseError("foreach requiere un identificador de lista/mapa");
            }
            std::string iterableName = iterVar ? iterVar->getName() : "";
            if (!match(TokenType::RParen)) {
                parseError("se esperaba ')' en foreach");
            }
            Token braceTok = forTok;
            if (match(TokenType::LBrace)) {
                braceTok = tokens[pos - 1];
            } else {
                parseError("se esperaba '{' en foreach");
            }
            auto body = std::make_unique<BlockStmt>();
            body->setLocation(braceTok.line, braceTok.column);
            parseStatements(body->statements, true);

            std::string idxName = "__fe_i" + std::to_string(syntheticCounter++);
            auto initValue = std::make_unique<NumberExpr>(0);
            initValue->setLocation(forTok.line, forTok.column);
            auto init = std::make_unique<VarDeclStmt>("jakhüwi", idxName, std::move(initValue));
            init->setLocation(forTok.line, forTok.column);

            std::vector<std::unique_ptr<Expr>> lenArgs;
            auto iterForLen = std::make_unique<VariableExpr>(iterableName);
            iterForLen->setLocation(forTok.line, forTok.column);
            lenArgs.push_back(std::move(iterForLen));
            auto lenCall = std::make_unique<CallExpr>("largo", std::move(lenArgs));
            lenCall->setLocation(forTok.line, forTok.column);

            auto idxForCond = std::make_unique<VariableExpr>(idxName);
            idxForCond->setLocation(forTok.line, forTok.column);
            auto cond = std::make_unique<BinaryExpr>('<', std::move(idxForCond), std::move(lenCall));
            cond->setLocation(forTok.line, forTok.column);

            auto idxPlus = std::make_unique<VariableExpr>(idxName);
            idxPlus->setLocation(forTok.line, forTok.column);
            auto one = std::make_unique<NumberExpr>(1);
            one->setLocation(forTok.line, forTok.column);
            auto sum = std::make_unique<BinaryExpr>('+', std::move(idxPlus), std::move(one));
            sum->setLocation(forTok.line, forTok.column);
            auto post = std::make_unique<AssignStmt>(idxName, std::move(sum));
            post->setLocation(forTok.line, forTok.column);

            auto iterForIndex = std::make_unique<VariableExpr>(iterableName);
            iterForIndex->setLocation(forTok.line, forTok.column);
            auto idxForIndex = std::make_unique<VariableExpr>(idxName);
            idxForIndex->setLocation(forTok.line, forTok.column);
            auto idxExpr = std::make_unique<IndexExpr>(std::move(iterForIndex), std::move(idxForIndex));
            idxExpr->setLocation(forTok.line, forTok.column);
            auto itemDecl = std::make_unique<VarDeclStmt>(itemType, itemName, std::move(idxExpr));
            itemDecl->setLocation(forTok.line, forTok.column);
            body->statements.insert(body->statements.begin(), std::move(itemDecl));

            auto node = std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(post), std::move(body));
            node->setLocation(forTok.line, forTok.column);
            return node;
        }
        // Not a foreach; rollback and parse normal for.
        pos = startPos;
    }

    std::unique_ptr<Stmt> init = nullptr;
    if (!match(TokenType::Semicolon)) {
        if (match(TokenType::KeywordDeclare)) {
            Token declTok = tokens[pos-1];
            std::string type;
            if (match(TokenType::KeywordTypeNumber) || match(TokenType::KeywordTypeString) ||
                match(TokenType::KeywordTypeBool) || match(TokenType::KeywordTypeList) ||
                match(TokenType::KeywordTypeMap) || match(TokenType::KeywordTrue)) {
                type = normalizeTypeNameLocal(tokens[pos-1]);
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
        } else if (peek().type == TokenType::Identifier &&
                   pos + 1 < tokens.size() &&
                   tokens[pos + 1].type == TokenType::Equal) {
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
        if (peek().type == TokenType::Identifier &&
            pos + 1 < tokens.size() &&
            tokens[pos + 1].type == TokenType::Equal) {
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

    if (!match(TokenType::RParen)) {
        parseError("se esperaba ')' al cerrar encabezado de 'taki'");
    }
    Token braceTok = forTok;
    if (match(TokenType::LBrace)) {
        braceTok = tokens[pos - 1];
    } else {
        parseError("se esperaba '{' en bloque de 'taki'");
    }
    auto body = std::make_unique<BlockStmt>();
    body->setLocation(braceTok.line, braceTok.column);
    parseStatements(body->statements, true);
    auto node = std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(post), std::move(body));
    node->setLocation(forTok.line, forTok.column);
    return node;
}

std::unique_ptr<Stmt> Parser::parseFunctionStatement(const Token &funcTok) {
    std::string name = "";
    if (peek().type == TokenType::Identifier) name = get().text;
    match(TokenType::LParen);
    std::vector<Param> params;
    if (peek().type != TokenType::RParen) {
        Param param;
        param.type = parseTypeName();
        if (param.type.empty()) {
            parseError("se esperaba un tipo de parametro");
        }
        if (peek().type == TokenType::Identifier) {
            param.name = get().text;
        } else {
            parseError("se esperaba un nombre de parametro");
        }
        params.push_back(param);
        while (match(TokenType::Comma)) {
            Param next;
            next.type = parseTypeName();
            if (next.type.empty()) {
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
        returnType = parseTypeName();
        if (returnType.empty()) {
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

std::unique_ptr<Stmt> Parser::parseExpressionOrAssignmentStatement() {
    auto expr = parseExpression();
    if (match(TokenType::Equal)) {
        auto value = parseExpression();
        match(TokenType::Semicolon);
        if (auto *var = dynamic_cast<VariableExpr*>(expr.get())) {
            auto node = std::make_unique<AssignStmt>(var->getName(), std::move(value));
            node->setLocation(var->getLine(), var->getColumn());
            return node;
        }
        if (auto *idx = dynamic_cast<IndexExpr*>(expr.get())) {
            auto node = std::make_unique<IndexAssignStmt>(idx->takeBase(),
                                                          idx->takeIndex(),
                                                          std::move(value));
            node->setLocation(idx->getLine(), idx->getColumn());
            return node;
        }
        if (auto *mem = dynamic_cast<MemberExpr*>(expr.get())) {
            auto key = std::make_unique<StringExpr>(mem->getMember());
            auto node = std::make_unique<IndexAssignStmt>(mem->takeBase(),
                                                          std::move(key),
                                                          std::move(value));
            node->setLocation(mem->getLine(), mem->getColumn());
            return node;
        }
        parseError("asignacion invalida");
    }
    size_t line = expr ? expr->getLine() : 0;
    size_t column = expr ? expr->getColumn() : 0;
    match(TokenType::Semicolon);
    auto node = std::make_unique<ExprStmt>(std::move(expr));
    node->setLocation(line, column);
    return node;
}

} // namespace aym
