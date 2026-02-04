#include "parser.h"

namespace aym {

std::unique_ptr<Stmt> Parser::parseClassStatement() {
    Token classTok = tokens[pos-1];
    std::string name;
    if (peek().type == TokenType::Identifier) {
        name = get().text;
    } else {
        parseError("se esperaba el nombre de la clase");
    }
    std::string baseName;
    if (match(TokenType::KeywordExtends)) {
        if (peek().type == TokenType::Identifier) {
            baseName = get().text;
        } else {
            parseError("se esperaba el nombre de la clase base");
        }
    }
    if (!match(TokenType::LBrace)) {
        parseError("se esperaba '{' despues del nombre de la clase");
    }
    auto node = parseClassBody(name, baseName, classTok);
    return node;
}

std::unique_ptr<ClassStmt> Parser::parseClassBody(const std::string &name,
                                                  const std::string &baseName,
                                                  const Token &startTok) {
    std::vector<ClassStmt::FieldDecl> fields;
    std::vector<ClassStmt::MethodDecl> methods;
    std::vector<ClassStmt::CtorDecl> ctors;
    while (pos < tokens.size() && peek().type != TokenType::EndOfFile) {
        if (match(TokenType::RBrace)) {
            break;
        }
        if (match(TokenType::Semicolon)) {
            continue;
        }
        bool isStatic = false;
        bool isPrivate = false;
        bool isOverride = false;
        while (true) {
            if (match(TokenType::KeywordStatic)) {
                isStatic = true;
            } else if (match(TokenType::KeywordPrivate)) {
                isPrivate = true;
            } else if (match(TokenType::KeywordPublic)) {
                // no-op, default public
            } else if (match(TokenType::KeywordOverride)) {
                isOverride = true;
            } else {
                break;
            }
        }

        if (match(TokenType::KeywordDeclare)) {
            ClassStmt::FieldDecl field;
            field.isStatic = isStatic;
            field.isPrivate = isPrivate;
            field.type = parseTypeName();
            if (field.type.empty()) {
                parseError("se esperaba un tipo para el atributo");
            }
            if (peek().type == TokenType::Identifier) {
                field.name = get().text;
            } else {
                parseError("se esperaba un nombre de atributo");
            }
            if (match(TokenType::Equal)) {
                field.init = parseExpression();
            }
            if (!match(TokenType::Semicolon)) {
                parseError("se esperaba ';' despues del atributo");
            }
            fields.push_back(std::move(field));
            continue;
        }

        if (match(TokenType::KeywordFunc)) {
            ClassStmt::MethodDecl method;
            method.isStatic = isStatic;
            method.isPrivate = isPrivate;
            method.isOverride = isOverride;
            if (peek().type == TokenType::Identifier || peek().type == TokenType::KeywordGetter ||
                peek().type == TokenType::KeywordSetter) {
                method.name = get().text;
            } else {
                parseError("se esperaba nombre de metodo");
            }
            if (!match(TokenType::LParen)) {
                parseError("se esperaba '(' en metodo");
            }
            if (peek().type != TokenType::RParen) {
                Param param;
                param.type = parseTypeName();
                if (param.type.empty()) {
                    parseError("se esperaba tipo de parametro");
                }
                if (peek().type == TokenType::Identifier) {
                    param.name = get().text;
                } else {
                    parseError("se esperaba nombre de parametro");
                }
                method.params.push_back(param);
                while (match(TokenType::Comma)) {
                    Param next;
                    next.type = parseTypeName();
                    if (next.type.empty()) {
                        parseError("se esperaba tipo de parametro");
                    }
                    if (peek().type == TokenType::Identifier) {
                        next.name = get().text;
                    } else {
                        parseError("se esperaba nombre de parametro");
                    }
                    method.params.push_back(next);
                }
            }
            match(TokenType::RParen);
            if (match(TokenType::Colon)) {
                method.returnType = parseTypeName();
                if (method.returnType.empty()) {
                    parseError("se esperaba tipo de retorno");
                }
            }
            if (!match(TokenType::LBrace)) {
                parseError("se esperaba '{' en metodo");
            }
            Token bodyTok = tokens[pos-1];
            auto body = std::make_unique<BlockStmt>();
            body->setLocation(bodyTok.line, bodyTok.column);
            parseStatements(body->statements, true);
            method.body = std::move(body);
            methods.push_back(std::move(method));
            continue;
        }

        if (match(TokenType::KeywordStart)) {
            ClassStmt::CtorDecl ctor;
            if (!match(TokenType::LParen)) {
                parseError("se esperaba '(' en constructor");
            }
            if (peek().type != TokenType::RParen) {
                Param param;
                param.type = parseTypeName();
                if (param.type.empty()) {
                    parseError("se esperaba tipo de parametro");
                }
                if (peek().type == TokenType::Identifier) {
                    param.name = get().text;
                } else {
                    parseError("se esperaba nombre de parametro");
                }
                ctor.params.push_back(param);
                while (match(TokenType::Comma)) {
                    Param next;
                    next.type = parseTypeName();
                    if (next.type.empty()) {
                        parseError("se esperaba tipo de parametro");
                    }
                    if (peek().type == TokenType::Identifier) {
                        next.name = get().text;
                    } else {
                        parseError("se esperaba nombre de parametro");
                    }
                    ctor.params.push_back(next);
                }
            }
            match(TokenType::RParen);
            if (!match(TokenType::LBrace)) {
                parseError("se esperaba '{' en constructor");
            }
            Token bodyTok = tokens[pos-1];
            auto body = std::make_unique<BlockStmt>();
            body->setLocation(bodyTok.line, bodyTok.column);
            parseStatements(body->statements, true);
            ctor.body = std::move(body);
            ctors.push_back(std::move(ctor));
            continue;
        }

        parseError("se esperaba atributo o metodo en la clase");
    }

    auto node = std::make_unique<ClassStmt>(name, baseName, std::move(fields), std::move(methods), std::move(ctors));
    node->setLocation(startTok.line, startTok.column);
    return node;
}

std::unique_ptr<Expr> Parser::parseClassPrimary() {
    if (match(TokenType::KeywordNew)) {
        Token tok = tokens[pos-1];
        if (peek().type != TokenType::Identifier) {
            parseError("se esperaba nombre de clase despues de 'machaqa'");
            return nullptr;
        }
        std::string name = get().text;
        std::vector<std::unique_ptr<Expr>> args;
        if (match(TokenType::LParen)) {
            args = parseArguments();
            match(TokenType::RParen);
        }
        auto node = std::make_unique<NewExpr>(name, std::move(args));
        node->setLocation(tok.line, tok.column);
        return node;
    }
    if (match(TokenType::KeywordSuper)) {
        Token tok = tokens[pos-1];
        auto node = std::make_unique<SuperExpr>();
        node->setLocation(tok.line, tok.column);
        return node;
    }
    if (match(TokenType::KeywordThis)) {
        Token idTok = tokens[pos-1];
        auto node = std::make_unique<VariableExpr>("Aka");
        node->setLocation(idTok.line, idTok.column);
        return node;
    }
    return nullptr;
}

} // namespace aym
