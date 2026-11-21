#include "parser.h"
#include <memory>
#include <string>
#include <algorithm>
#include <unordered_set>
#include "../utils/diagnostic.h"

namespace aym {

namespace {

std::string canonicalTypeName(const Token &tok) {
    switch (tok.type) {
        case TokenType::KeywordTypeNumber:
            return "jakhüwi";
        case TokenType::KeywordTypeString:
            return "aru";
        case TokenType::KeywordTypeBool:
        case TokenType::KeywordTrue:
            return "chiqa";
        case TokenType::KeywordTypeList:
            return "t'aqa";
        case TokenType::KeywordTypeMap:
            return "mapa";
        default:
            return tok.text;
    }
}

std::unique_ptr<Expr> cloneSimpleExpr(const Expr *expr) {
    if (!expr) return nullptr;
    if (auto *v = dynamic_cast<const VariableExpr*>(expr)) {
        auto node = std::make_unique<VariableExpr>(v->getName());
        node->setLocation(v->getLine(), v->getColumn());
        return node;
    }
    if (auto *n = dynamic_cast<const NumberExpr*>(expr)) {
        auto node = std::make_unique<NumberExpr>(n->getValue());
        node->setLocation(n->getLine(), n->getColumn());
        return node;
    }
    if (auto *s = dynamic_cast<const StringExpr*>(expr)) {
        auto node = std::make_unique<StringExpr>(s->getValue());
        node->setLocation(s->getLine(), s->getColumn());
        return node;
    }
    if (auto *b = dynamic_cast<const BoolExpr*>(expr)) {
        auto node = std::make_unique<BoolExpr>(b->getValue());
        node->setLocation(b->getLine(), b->getColumn());
        return node;
    }
    if (auto *m = dynamic_cast<const MemberExpr*>(expr)) {
        auto base = cloneSimpleExpr(m->getBase());
        if (!base) return nullptr;
        auto node = std::make_unique<MemberExpr>(std::move(base), m->getMember());
        node->setLocation(m->getLine(), m->getColumn());
        return node;
    }
    if (auto *idx = dynamic_cast<const IndexExpr*>(expr)) {
        auto base = cloneSimpleExpr(idx->getBase());
        auto index = cloneSimpleExpr(idx->getIndex());
        if (!base || !index) return nullptr;
        auto node = std::make_unique<IndexExpr>(std::move(base), std::move(index));
        node->setLocation(idx->getLine(), idx->getColumn());
        return node;
    }
    return nullptr;
}

} // namespace

Parser::Parser(const std::vector<Token>& t, DiagnosticEngine *diag)
    : tokens(t), diagnostics(diag) {}

std::string Parser::parseTypeName() {
    if (match(TokenType::KeywordTypeNumber) || match(TokenType::KeywordTypeString) ||
        match(TokenType::KeywordTypeBool) || match(TokenType::KeywordTrue)) {
        return canonicalTypeName(tokens[pos-1]);
    }
    if (match(TokenType::KeywordTypeList) || match(TokenType::KeywordTypeMap)) {
        std::string base = canonicalTypeName(tokens[pos-1]);
        if (match(TokenType::Less)) {
            std::string elemType = parseTypeName();
            if (elemType.empty()) {
                parseError("se esperaba tipo de elemento en tipo generico");
            }
            if (!match(TokenType::Greater)) {
                parseError("se esperaba '>' en tipo generico");
            }
            return (base == "t'aqa" ? "t'aqa:" : "mapa:") + elemType;
        }
        return base;
    }
    if (match(TokenType::Identifier)) {
        return tokens[pos-1].text;
    }
    return "";
}

std::vector<std::unique_ptr<Node>> Parser::parse() {
    syntheticFunctions.clear();
    std::vector<std::unique_ptr<Stmt>> stmts;
    bool hasStart = match(TokenType::KeywordStart);
    parseStatements(stmts);
    if (match(TokenType::KeywordEnd)) {
        if (peek().type != TokenType::EndOfFile) {
            parseError("token inesperado despues de 'tukuya'");
        }
    } else if (hasStart) {
        parseError("se esperaba 'tukuya' al final del programa");
    }
    std::vector<std::unique_ptr<Node>> nodes;
    for (auto &s : stmts) nodes.push_back(std::move(s));
    for (auto &fn : syntheticFunctions) nodes.push_back(std::move(fn));
    syntheticFunctions.clear();
    return nodes;
}

std::unique_ptr<Expr> Parser::parseExpressionOnly() {
    auto expr = parseExpression();
    if (!expr) {
        match(TokenType::Semicolon);
        auto node = std::make_unique<NumberExpr>(0);
        node->setLocation(tokens[pos-1].line, tokens[pos-1].column);
        return node;
    }
    if (peek().type != TokenType::EndOfFile) {
        parseError("token inesperado en expresion");
    }
    return expr;
}

const Token &Parser::peek() const { return tokens[pos]; }
const Token &Parser::get() { return tokens[pos++]; }

bool Parser::match(TokenType type) {
    if (peek().type == type) { get(); return true; }
    return false;
}

void Parser::parseError(const std::string &msg) {
    const Token &tok = peek();
    if (diagnostics) {
        diagnostics->error("AYM2001", msg, tok.line, tok.column);
    } else {
        std::cerr << "[parser] Error en linea " << tok.line << ", columna " << tok.column
                  << ": " << msg << std::endl;
    }
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
        size_t startPos = pos;
        auto stmt = parseSingleStatement();
        if (stmt) {
            nodes.push_back(std::move(stmt));
        }
        if (pos == startPos) {
            const Token &tok = peek();
            if (diagnostics) {
                diagnostics->error("AYM2002",
                                   "recuperacion forzada para evitar bucle infinito",
                                   tok.line, tok.column);
            } else {
                std::cerr << "[parser] Error en linea " << tok.line << ", columna " << tok.column
                          << ": recuperacion forzada para evitar bucle infinito" << std::endl;
            }
            hadError = true;
            if (tok.type == TokenType::EndOfFile) {
                break;
            }
            get();
        }
    }
}

std::unique_ptr<Stmt> Parser::parseSingleStatement() {
    if (match(TokenType::KeywordClass)) {
        return parseClassStatement();
    }
    if (match(TokenType::KeywordImport)) {
        return parseImportStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordEnum)) {
        return parseEnumStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordMatch)) {
        return parseMatchStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordThrow)) {
        return parseThrowStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordTry)) {
        return parseTryStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordDeclare)) {
        return parseVarDeclStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordConst)) {
        Token constTok = tokens[pos-1];
        match(TokenType::KeywordDeclare);
        return parseVarDeclStatement(constTok, true);
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
        return parsePrintStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordIf)) {
        return parseIfStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordWhile)) {
        return parseWhileStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordDo)) {
        return parseDoWhileStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordFor)) {
        return parseForStatement(tokens[pos-1]);
    }
    if (match(TokenType::KeywordFunc)) {
        return parseFunctionStatement(tokens[pos-1]);
    }

    // Bloque suelto '{ ... }' con su propio ambito (gramatica: sentencia = bloque).
    // Un literal de mapa como sentencia no tendria efecto, asi que '{' al inicio
    // de una sentencia siempre abre un bloque.
    if (match(TokenType::LBrace)) {
        auto block = std::make_unique<BlockStmt>();
        block->setLocation(tokens[pos-1].line, tokens[pos-1].column);
        parseStatements(block->statements, true);
        return block;
    }

    if (match(TokenType::Semicolon)) {
        auto node = std::make_unique<ExprStmt>(nullptr);
        node->setLocation(tokens[pos-1].line, tokens[pos-1].column);
        return node;
    }

    return parseExpressionOrAssignmentStatement();
}

std::unique_ptr<Stmt> Parser::parseVarDeclStatement(const Token &declTok, bool initialConst) {
    bool isConst = initialConst || match(TokenType::KeywordConst);
    std::string type = parseTypeName();
    if (type.empty()) {
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
    node->setConst(isConst);
    node->setLocation(declTok.line, declTok.column);
    return node;
}

std::unique_ptr<Stmt> Parser::parsePrintStatement(const Token &printTok) {
    std::vector<std::unique_ptr<Expr>> exprs;
    std::unique_ptr<Expr> separator;
    std::unique_ptr<Expr> terminator;
    match(TokenType::LParen);
    if (peek().type != TokenType::RParen) {
        while (true) {
            if ((peek().type == TokenType::Identifier || peek().type == TokenType::KeywordTypeList) &&
                pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::Equal) {
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
    node->setLocation(printTok.line, printTok.column);
    return node;
}

std::unique_ptr<Stmt> Parser::parseIfStatement(const Token &ifTok) {
    if (!match(TokenType::LParen)) {
        parseError("se esperaba '(' despues de 'ukaxa'");
    }
    auto cond = parseExpression();
    if (!match(TokenType::RParen)) {
        parseError("se esperaba ')' despues de la condicion de 'ukaxa'");
    }
    if (!match(TokenType::LBrace)) {
        parseError("se esperaba '{' en bloque de 'ukaxa'");
    }
    Token thenTok = tokens[pos > 0 ? pos - 1 : pos];
    auto thenBlock = std::make_unique<BlockStmt>();
    thenBlock->setLocation(thenTok.line, thenTok.column);
    parseStatements(thenBlock->statements, true);
    std::unique_ptr<BlockStmt> elseBlock;
    if (match(TokenType::KeywordElse)) {
        elseBlock = std::make_unique<BlockStmt>();
        Token elseTok = tokens[pos - 1];
        elseBlock->setLocation(elseTok.line, elseTok.column);
        if (match(TokenType::KeywordIf)) {
            Token nestedIfTok = tokens[pos - 1];
            elseBlock->statements.push_back(parseIfStatement(nestedIfTok));
        } else if (match(TokenType::LBrace)) {
            Token elseBrace = tokens[pos - 1];
            elseBlock->setLocation(elseBrace.line, elseBrace.column);
            parseStatements(elseBlock->statements, true);
        } else {
            parseError("se esperaba '{' o 'ukaxa' despues de 'maysatxa'");
        }
    }
    auto node = std::make_unique<IfStmt>(std::move(cond), std::move(thenBlock), std::move(elseBlock));
    node->setLocation(ifTok.line, ifTok.column);
    return node;
}

std::unique_ptr<Stmt> Parser::parseWhileStatement(const Token &whileTok) {
    if (!match(TokenType::LParen)) {
        parseError("se esperaba '(' despues de 'ukhakamaxa'");
    }
    auto cond = parseExpression();
    if (!match(TokenType::RParen)) {
        parseError("se esperaba ')' despues de la condicion de 'ukhakamaxa'");
    }
    Token braceTok = whileTok;
    if (match(TokenType::LBrace)) {
        braceTok = tokens[pos - 1];
    } else {
        parseError("se esperaba '{' en bloque de 'ukhakamaxa'");
    }
    auto block = std::make_unique<BlockStmt>();
    block->setLocation(braceTok.line, braceTok.column);
    parseStatements(block->statements, true);
    auto node = std::make_unique<WhileStmt>(std::move(cond), std::move(block));
    node->setLocation(whileTok.line, whileTok.column);
    return node;
}

std::unique_ptr<Stmt> Parser::parseDoWhileStatement(const Token &doTok) {
    Token braceTok = doTok;
    if (match(TokenType::LBrace)) {
        braceTok = tokens[pos - 1];
    } else {
        parseError("se esperaba '{' despues de 'luraña'");
    }
    auto block = std::make_unique<BlockStmt>();
    block->setLocation(braceTok.line, braceTok.column);
    parseStatements(block->statements, true);
    if (!match(TokenType::KeywordWhile)) {
        parseError("se esperaba 'ukhakamaxa' despues del bloque de 'luraña'");
    }
    if (!match(TokenType::LParen)) {
        parseError("se esperaba '(' despues de 'ukhakamaxa'");
    }
    auto cond = parseExpression();
    if (!match(TokenType::RParen)) {
        parseError("se esperaba ')' despues de la condicion de 'ukhakamaxa'");
    }
    if (!match(TokenType::Semicolon)) {
        parseError("se esperaba ';' despues de 'luraña ... ukhakamaxa (...)'");
    }
    auto node = std::make_unique<DoWhileStmt>(std::move(block), std::move(cond));
    node->setLocation(doTok.line, doTok.column);
    return node;
}

std::unique_ptr<Stmt> Parser::parseForStatement(const Token &forTok) {
    if (!match(TokenType::LParen)) {
        parseError("se esperaba '(' despues de 'kuti'");
    }
    // foreach desugaring: kuti(yatiya tipo item: listaIdentificador) { ... }
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
            std::string type = parseTypeName();
            if (type.empty()) {
                parseError("se esperaba un tipo en el encabezado de 'kuti'");
            }
            std::string name;
            if (peek().type == TokenType::Identifier) {
                name = get().text;
            } else {
                parseError("se esperaba un identificador en el encabezado de 'kuti'");
            }
            std::unique_ptr<Expr> initExpr;
            if (match(TokenType::Equal)) initExpr = parseExpression();
            init = std::make_unique<VarDeclStmt>(type, name, std::move(initExpr));
            init->setLocation(declTok.line, declTok.column);
        } else {
            init = parseAssignmentOrExprStatement(false);
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
        post = parseAssignmentOrExprStatement(false);
        match(TokenType::Semicolon);
    }
    if (!post) post = std::make_unique<ExprStmt>(nullptr);

    if (!match(TokenType::RParen)) {
        parseError("se esperaba ')' al cerrar encabezado de 'kuti'");
    }
    Token braceTok = forTok;
    if (match(TokenType::LBrace)) {
        braceTok = tokens[pos - 1];
    } else {
        parseError("se esperaba '{' en bloque de 'kuti'");
    }
    auto body = std::make_unique<BlockStmt>();
    body->setLocation(braceTok.line, braceTok.column);
    parseStatements(body->statements, true);
    auto node = std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(post), std::move(body));
    node->setLocation(forTok.line, forTok.column);
    return node;
}

void Parser::parseParamList(std::vector<Param> &params) {
    if (peek().type == TokenType::RParen) {
        return;
    }
    bool seenDefault = false;
    while (true) {
        Param param;
        param.type = parseTypeName();
        if (param.type.empty()) {
            parseError("se esperaba un tipo de parametro");
            break;
        }
        if (peek().type == TokenType::Identifier) {
            param.name = get().text;
        } else {
            parseError("se esperaba un nombre de parametro");
            break;
        }
        if (match(TokenType::Equal)) {
            param.defaultValue = std::shared_ptr<Expr>(parseExpression());
            seenDefault = true;
        } else if (seenDefault) {
            parseError("un parametro sin valor por defecto no puede ir despues de uno con valor por defecto");
        }
        params.push_back(std::move(param));
        if (!match(TokenType::Comma)) {
            break;
        }
    }
}

std::unique_ptr<Stmt> Parser::parseFunctionStatement(const Token &funcTok) {
    std::string name = "";
    if (peek().type == TokenType::Identifier) name = get().text;
    match(TokenType::LParen);
    std::vector<Param> params;
    parseParamList(params);
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

std::unique_ptr<Stmt> Parser::parseAssignmentOrExprStatement(bool consumeSemicolon) {
    auto expr = parseExpression();
    if (match(TokenType::Equal)) {
        auto value = parseExpression();
        if (consumeSemicolon) match(TokenType::Semicolon);
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
            key->setLocation(mem->getLine(), mem->getColumn());
            auto node = std::make_unique<IndexAssignStmt>(mem->takeBase(),
                                                          std::move(key),
                                                          std::move(value));
            node->setLocation(mem->getLine(), mem->getColumn());
            return node;
        }
        parseError("asignacion invalida");
        return nullptr;
    }
    {
        char compoundOp = 0;
        Token opTok = peek();
        if (match(TokenType::PlusEqual)) compoundOp = '+';
        else if (match(TokenType::MinusEqual)) compoundOp = '-';
        else if (match(TokenType::StarEqual)) compoundOp = '*';
        else if (match(TokenType::SlashEqual)) compoundOp = '/';
        else if (match(TokenType::PercentEqual)) compoundOp = '%';
        if (compoundOp) {
            auto rhs = parseExpression();
            if (consumeSemicolon) match(TokenType::Semicolon);
            if (auto *var = dynamic_cast<VariableExpr*>(expr.get())) {
                auto lhs = std::make_unique<VariableExpr>(var->getName());
                lhs->setLocation(var->getLine(), var->getColumn());
                auto value = std::make_unique<BinaryExpr>(compoundOp, std::move(lhs), std::move(rhs));
                value->setLocation(opTok.line, opTok.column);
                auto node = std::make_unique<AssignStmt>(var->getName(), std::move(value));
                node->setLocation(var->getLine(), var->getColumn());
                return node;
            }
            if (auto *idx = dynamic_cast<IndexExpr*>(expr.get())) {
                auto clonedBase = cloneSimpleExpr(idx->getBase());
                auto clonedIdx = cloneSimpleExpr(idx->getIndex());
                if (clonedBase && clonedIdx) {
                    auto lhs = std::make_unique<IndexExpr>(std::move(clonedBase), std::move(clonedIdx));
                    lhs->setLocation(idx->getLine(), idx->getColumn());
                    auto value = std::make_unique<BinaryExpr>(compoundOp, std::move(lhs), std::move(rhs));
                    value->setLocation(opTok.line, opTok.column);
                    auto node = std::make_unique<IndexAssignStmt>(idx->takeBase(),
                                                                  idx->takeIndex(),
                                                                  std::move(value));
                    node->setLocation(idx->getLine(), idx->getColumn());
                    return node;
                }
            }
            if (auto *mem = dynamic_cast<MemberExpr*>(expr.get())) {
                auto clonedBase = cloneSimpleExpr(mem->getBase());
                if (clonedBase) {
                    auto lhs = std::make_unique<MemberExpr>(std::move(clonedBase), mem->getMember());
                    lhs->setLocation(mem->getLine(), mem->getColumn());
                    auto value = std::make_unique<BinaryExpr>(compoundOp, std::move(lhs), std::move(rhs));
                    value->setLocation(opTok.line, opTok.column);
                    auto key = std::make_unique<StringExpr>(mem->getMember());
                    key->setLocation(mem->getLine(), mem->getColumn());
                    auto node = std::make_unique<IndexAssignStmt>(mem->takeBase(),
                                                                  std::move(key),
                                                                  std::move(value));
                    node->setLocation(mem->getLine(), mem->getColumn());
                    return node;
                }
            }
            parseError("asignacion compuesta solo admite variables, indices o campos simples");
            return nullptr;
        }
    }
    size_t line = expr ? expr->getLine() : 0;
    size_t column = expr ? expr->getColumn() : 0;
    if (consumeSemicolon) match(TokenType::Semicolon);
    auto node = std::make_unique<ExprStmt>(std::move(expr));
    node->setLocation(line, column);
    return node;
}

std::unique_ptr<Stmt> Parser::parseExpressionOrAssignmentStatement() {
    return parseAssignmentOrExprStatement(true);
}

std::vector<std::string> Parser::parseImportSymbols() {
    std::vector<std::string> symbols;
    if (match(TokenType::LBracket)) {
        if (peek().type != TokenType::RBracket) {
            while (true) {
                if (peek().type == TokenType::String || peek().type == TokenType::Identifier) {
                    symbols.push_back(get().text);
                } else {
                    parseError("se esperaba nombre de simbolo en la lista de importacion");
                    break;
                }
                if (!match(TokenType::Comma)) break;
            }
        }
        if (!match(TokenType::RBracket)) {
            parseError("se esperaba ']' en la lista de importacion");
        }
        return symbols;
    }

    if (peek().type == TokenType::String || peek().type == TokenType::Identifier) {
        symbols.push_back(get().text);
        return symbols;
    }

    parseError("se esperaba simbolo o lista de simbolos despues de ',' en 'apnaq'");
    return symbols;
}

std::vector<std::pair<std::string,std::string>> Parser::parseImportAliases() {
    std::vector<std::pair<std::string,std::string>> aliases;
    if (!match(TokenType::LBrace)) {
        parseError("se esperaba '{' para aliases en 'apnaq'");
        return aliases;
    }
    if (peek().type != TokenType::RBrace) {
        while (true) {
            if (!(peek().type == TokenType::String || peek().type == TokenType::Identifier)) {
                parseError("se esperaba nombre origen en alias de importacion");
                break;
            }
            std::string from = get().text;
            if (!match(TokenType::Colon)) {
                parseError("se esperaba ':' en alias de importacion");
                break;
            }
            if (!(peek().type == TokenType::String || peek().type == TokenType::Identifier)) {
                parseError("se esperaba nombre destino en alias de importacion");
                break;
            }
            std::string to = get().text;
            aliases.push_back({from, to});
            if (!match(TokenType::Comma)) break;
        }
    }
    if (!match(TokenType::RBrace)) {
        parseError("se esperaba '}' en alias de importacion");
    }
    return aliases;
}

std::unique_ptr<Stmt> Parser::parseImportStatement(const Token &importTok) {
    std::string moduleName;
    std::vector<std::string> symbols;
    std::vector<std::pair<std::string,std::string>> aliases;
    if (match(TokenType::LParen)) {
        if (peek().type == TokenType::String || peek().type == TokenType::Identifier) {
            moduleName = get().text;
        } else {
            parseError("se esperaba la ruta del modulo despues de 'apnaq'");
        }
        if (match(TokenType::Comma)) {
            if (peek().type == TokenType::LBrace) {
                aliases = parseImportAliases();
            } else {
                symbols = parseImportSymbols();
            }
        }
        match(TokenType::RParen);
    } else {
        parseError("se esperaba '(' despues de 'apnaq'");
    }
    if (!match(TokenType::Semicolon)) {
        parseError("se esperaba ';' despues de la declaracion de modulo");
    }
    auto node = std::make_unique<ImportStmt>(moduleName, std::move(symbols), std::move(aliases));
    node->setLocation(importTok.line, importTok.column);
    return node;
}

namespace {
// Evalua en tiempo de compilacion el valor de un miembro de 'siqicha' para
// poder numerar correctamente los miembros siguientes (A = -1, B -> 0).
bool evalEnumConstant(const aym::Expr *e, long long &out) {
    using namespace aym;
    if (auto *n = dynamic_cast<const NumberExpr*>(e)) { out = n->getValue(); return true; }
    if (auto *u = dynamic_cast<const UnaryExpr*>(e)) {
        long long v;
        if (!evalEnumConstant(u->getExpr(), v)) return false;
        switch (u->getOp()) {
            case '-': out = -v; return true;
            case '+': out = v; return true;
            case '~': out = ~v; return true;
            default: return false;
        }
    }
    if (auto *b = dynamic_cast<const BinaryExpr*>(e)) {
        long long l, r;
        if (!evalEnumConstant(b->getLeft(), l) || !evalEnumConstant(b->getRight(), r)) return false;
        switch (b->getOp()) {
            case '+': out = l + r; return true;
            case '-': out = l - r; return true;
            case '*': out = l * r; return true;
            case '/': if (r == 0) return false; out = l / r; return true;
            case '%': if (r == 0) return false; out = l % r; return true;
            case 'A': out = l & r; return true;
            case 'O': out = l | r; return true;
            case 'X': out = l ^ r; return true;
            case 'L': if (r < 0 || r > 63) return false; out = l << r; return true;
            case 'R': if (r < 0 || r > 63) return false; out = l >> r; return true;
            default: return false;
        }
    }
    return false;
}
} // namespace

std::unique_ptr<Stmt> Parser::parseEnumStatement(const Token &enumTok) {
    std::string enumName;
    if (peek().type == TokenType::Identifier) {
        enumName = get().text;
    } else {
        parseError("se esperaba nombre de 'siqicha'");
    }
    if (!match(TokenType::LBrace)) {
        parseError("se esperaba '{' en 'siqicha'");
    }

    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> items;
    std::unordered_set<std::string> seenMembers;
    long long nextValue = 0;
    while (peek().type != TokenType::RBrace && peek().type != TokenType::EndOfFile) {
        if (peek().type != TokenType::Identifier) {
            parseError("se esperaba nombre de miembro en 'siqicha'");
            break;
        }
        Token memberTok = get();
        std::string memberName = memberTok.text;
        if (seenMembers.count(memberName)) {
            parseError("miembro repetido en 'siqicha': '" + memberName + "'");
        }
        seenMembers.insert(memberName);

        std::unique_ptr<Expr> valueExpr;
        if (match(TokenType::Equal)) {
            valueExpr = parseExpression();
            long long value = 0;
            if (valueExpr && evalEnumConstant(valueExpr.get(), value)) {
                nextValue = value + 1;
            } else {
                parseError("el valor de '" + memberName +
                           "' en 'siqicha' debe ser una constante entera");
                nextValue += 1;
            }
        } else {
            auto num = std::make_unique<NumberExpr>(nextValue);
            num->setLocation(memberTok.line, memberTok.column);
            valueExpr = std::move(num);
            nextValue += 1;
        }

        auto keyExpr = std::make_unique<StringExpr>(memberName);
        keyExpr->setLocation(memberTok.line, memberTok.column);
        items.push_back({std::move(keyExpr), std::move(valueExpr)});

        if (!match(TokenType::Comma)) break;
    }

    if (!match(TokenType::RBrace)) {
        parseError("se esperaba '}' en 'siqicha'");
    }
    match(TokenType::Semicolon);

    auto mapExpr = std::make_unique<MapExpr>(std::move(items));
    mapExpr->setLocation(enumTok.line, enumTok.column);
    auto node = std::make_unique<VarDeclStmt>("mapa", enumName, std::move(mapExpr));
    node->setLocation(enumTok.line, enumTok.column);
    return node;
}

std::unique_ptr<Stmt> Parser::parseMatchStatement(const Token &matchTok) {
    if (!match(TokenType::LParen)) {
        parseError("se esperaba '(' despues de 'khiti'");
    }
    auto matchExpr = parseExpression();
    if (!match(TokenType::RParen)) {
        parseError("se esperaba ')' en 'khiti'");
    }
    if (!match(TokenType::LBrace)) {
        parseError("se esperaba '{' en 'khiti'");
    }

    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> cases;
    std::unique_ptr<BlockStmt> defaultCase;

    while (peek().type != TokenType::RBrace && peek().type != TokenType::EndOfFile) {
        if (match(TokenType::KeywordCase)) {
            Token caseTok = tokens[pos - 1];
            auto parseCaseValue = [&]() -> std::unique_ptr<Expr> {
                auto firstExpr = parseExpression();
                if (peek().type == TokenType::Dot &&
                    (pos + 1) < tokens.size() &&
                    tokens[pos + 1].type == TokenType::Dot) {
                    get();
                    get();
                    auto secondExpr = parseExpression();
                    std::vector<std::unique_ptr<Expr>> args;
                    args.push_back(std::move(firstExpr));
                    args.push_back(std::move(secondExpr));
                    auto rangeExpr = std::make_unique<CallExpr>("__rango_case__", std::move(args));
                    rangeExpr->setLocation(caseTok.line, caseTok.column);
                    return rangeExpr;
                }
                return firstExpr;
            };
            std::vector<std::unique_ptr<Expr>> caseAlternatives;
            caseAlternatives.push_back(parseCaseValue());
            while (match(TokenType::Comma)) {
                caseAlternatives.push_back(parseCaseValue());
            }
            std::unique_ptr<Expr> caseExpr;
            if (caseAlternatives.size() == 1) {
                caseExpr = std::move(caseAlternatives[0]);
            } else {
                auto listExpr = std::make_unique<ListExpr>(std::move(caseAlternatives));
                listExpr->setLocation(caseTok.line, caseTok.column);
                caseExpr = std::move(listExpr);
            }
            if (!match(TokenType::Colon)) {
                parseError("se esperaba ':' despues de 'kuna'");
            }
            auto caseBlock = std::make_unique<BlockStmt>();
            if (match(TokenType::LBrace)) {
                Token blockTok = tokens[pos - 1];
                caseBlock->setLocation(blockTok.line, blockTok.column);
                parseStatements(caseBlock->statements, true);
            } else {
                parseError("se esperaba '{' en bloque de 'kuna'");
            }
            auto breakNode = std::make_unique<BreakStmt>();
            breakNode->setLocation(caseTok.line, caseTok.column);
            caseBlock->statements.push_back(std::move(breakNode));
            cases.push_back({std::move(caseExpr), std::move(caseBlock)});
            continue;
        }

        if (match(TokenType::KeywordDefault)) {
            if (defaultCase) {
                parseError("solo se permite un bloque 'yaqha' en 'khiti'");
            }
            if (!match(TokenType::Colon)) {
                parseError("se esperaba ':' despues de 'yaqha'");
            }
            auto block = std::make_unique<BlockStmt>();
            if (match(TokenType::LBrace)) {
                Token blockTok = tokens[pos - 1];
                block->setLocation(blockTok.line, blockTok.column);
                parseStatements(block->statements, true);
            } else {
                parseError("se esperaba '{' en bloque de 'yaqha'");
            }
            defaultCase = std::move(block);
            continue;
        }

        parseError("se esperaba 'kuna' o 'yaqha' en 'khiti'");
        if (peek().type == TokenType::RBrace) break;
    }

    if (!match(TokenType::RBrace)) {
        parseError("se esperaba '}' al cerrar 'khiti'");
    }
    match(TokenType::Semicolon);

    auto node = std::make_unique<SwitchStmt>(std::move(matchExpr),
                                             std::move(cases),
                                             std::move(defaultCase));
    node->setLocation(matchTok.line, matchTok.column);
    return node;
}

std::unique_ptr<Stmt> Parser::parseThrowStatement(const Token &throwTok) {
    std::unique_ptr<Expr> typeExpr;
    std::unique_ptr<Expr> messageExpr;
    if (!match(TokenType::LParen)) {
        parseError("se esperaba '(' despues de 'pantja'");
    } else {
        if (peek().type != TokenType::RParen) {
            auto first = parseExpression();
            if (match(TokenType::Comma)) {
                typeExpr = std::move(first);
                messageExpr = parseExpression();
            } else {
                messageExpr = std::move(first);
            }
        } else {
            parseError("se esperaba mensaje en 'pantja'");
        }
        match(TokenType::RParen);
    }
    if (!match(TokenType::Semicolon)) {
        parseError("se esperaba ';' despues de 'pantja'");
    }
    auto node = std::make_unique<ThrowStmt>(std::move(typeExpr), std::move(messageExpr));
    node->setLocation(throwTok.line, throwTok.column);
    return node;
}

std::unique_ptr<Stmt> Parser::parseTryStatement(const Token &tryTok) {
    if (!match(TokenType::LBrace)) {
        parseError("se esperaba '{' despues de 'yant'aña'");
    }
    Token tryBlockTok = tokens[pos-1];
    auto tryBlock = std::make_unique<BlockStmt>();
    tryBlock->setLocation(tryBlockTok.line, tryBlockTok.column);
    parseStatements(tryBlock->statements, true);

    std::vector<TryStmt::CatchClause> catches;
    while (match(TokenType::KeywordCatch)) {
        if (!match(TokenType::LParen)) {
            parseError("se esperaba '(' despues de 'katjaña'");
        }
        std::string typeName;
        if (peek().type == TokenType::String) {
            typeName = get().text;
            if (!match(TokenType::Comma)) {
                parseError("se esperaba ',' despues del tipo en 'katjaña'");
            }
        }
        std::string varName;
        if (peek().type == TokenType::Identifier) {
            varName = get().text;
        } else {
            parseError("se esperaba identificador en 'katjaña'");
        }
        match(TokenType::RParen);
        if (!match(TokenType::LBrace)) {
            parseError("se esperaba '{' despues de 'katjaña'");
        }
        Token catchTok = tokens[pos-1];
        auto catchBlock = std::make_unique<BlockStmt>();
        catchBlock->setLocation(catchTok.line, catchTok.column);
        parseStatements(catchBlock->statements, true);
        TryStmt::CatchClause clause{typeName, varName, std::move(catchBlock)};
        catches.push_back(std::move(clause));
    }

    std::unique_ptr<BlockStmt> finallyBlock;
    if (match(TokenType::KeywordFinally)) {
        if (!match(TokenType::LBrace)) {
            parseError("se esperaba '{' despues de 'tukuyawi'");
        }
        Token finTok = tokens[pos-1];
        finallyBlock = std::make_unique<BlockStmt>();
        finallyBlock->setLocation(finTok.line, finTok.column);
        parseStatements(finallyBlock->statements, true);
    }

    if (catches.empty() && !finallyBlock) {
        parseError("se esperaba 'katjaña' o 'tukuyawi' despues de 'yant'aña'");
    }
    auto node = std::make_unique<TryStmt>(std::move(tryBlock), std::move(catches), std::move(finallyBlock));
    node->setLocation(tryTok.line, tryTok.column);
    return node;
}

} // namespace aym
