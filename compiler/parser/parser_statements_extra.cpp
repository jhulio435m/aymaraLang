#include "parser.h"
#include <unordered_set>

namespace aym {

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

std::unique_ptr<Stmt> Parser::parseEnumStatement(const Token &enumTok) {
    std::string enumName;
    if (peek().type == TokenType::Identifier) {
        enumName = get().text;
    } else {
        parseError("se esperaba nombre del enum");
    }
    if (!match(TokenType::LBrace)) {
        parseError("se esperaba '{' en enum");
    }

    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> items;
    std::unordered_set<std::string> seenMembers;
    long long nextValue = 0;
    while (peek().type != TokenType::RBrace && peek().type != TokenType::EndOfFile) {
        if (peek().type != TokenType::Identifier) {
            parseError("se esperaba nombre de miembro en enum");
            break;
        }
        Token memberTok = get();
        std::string memberName = memberTok.text;
        if (seenMembers.count(memberName)) {
            parseError("miembro enum repetido: '" + memberName + "'");
        }
        seenMembers.insert(memberName);

        std::unique_ptr<Expr> valueExpr;
        if (match(TokenType::Equal)) {
            valueExpr = parseExpression();
            if (auto *num = dynamic_cast<NumberExpr*>(valueExpr.get())) {
                nextValue = num->getValue() + 1;
            } else {
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
        parseError("se esperaba '}' en enum");
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
        parseError("se esperaba '(' despues de 'match'");
    }
    auto matchExpr = parseExpression();
    if (!match(TokenType::RParen)) {
        parseError("se esperaba ')' en match");
    }
    if (!match(TokenType::LBrace)) {
        parseError("se esperaba '{' en match");
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
                parseError("se esperaba ':' despues de case");
            }
            auto caseBlock = std::make_unique<BlockStmt>();
            if (match(TokenType::LBrace)) {
                Token blockTok = tokens[pos - 1];
                caseBlock->setLocation(blockTok.line, blockTok.column);
                parseStatements(caseBlock->statements, true);
            } else {
                parseError("se esperaba '{' en bloque de case");
            }
            auto breakNode = std::make_unique<BreakStmt>();
            breakNode->setLocation(caseTok.line, caseTok.column);
            caseBlock->statements.push_back(std::move(breakNode));
            cases.push_back({std::move(caseExpr), std::move(caseBlock)});
            continue;
        }

        if (match(TokenType::KeywordDefault)) {
            if (defaultCase) {
                parseError("solo se permite un bloque default en match");
            }
            if (!match(TokenType::Colon)) {
                parseError("se esperaba ':' despues de default");
            }
            auto block = std::make_unique<BlockStmt>();
            if (match(TokenType::LBrace)) {
                Token blockTok = tokens[pos - 1];
                block->setLocation(blockTok.line, blockTok.column);
                parseStatements(block->statements, true);
            } else {
                parseError("se esperaba '{' en bloque default");
            }
            defaultCase = std::move(block);
            continue;
        }

        parseError("se esperaba 'case' o 'default' en match");
        if (peek().type == TokenType::RBrace) break;
    }

    if (!match(TokenType::RBrace)) {
        parseError("se esperaba '}' al cerrar match");
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
