#include "lexer.h"
#include <cctype>
#include <stdexcept>

namespace aym {

bool Lexer::skipComment(size_t startLine, size_t startColumn) {
    if (peek() != '/' || pos + 1 >= src.size()) {
        return false;
    }
    if (src[pos + 1] == '/') { // line comment
        while (pos < src.size() && get() != '\n');
        return true;
    }
    if (src[pos + 1] == '*') { // block comment
        get(); get();
        while (pos + 1 < src.size() && !(peek() == '*' && src[pos + 1] == '/')) get();
        if (pos + 1 >= src.size()) {
            throw std::runtime_error(
                "Unterminated block comment starting at line " + std::to_string(startLine) +
                ", column " + std::to_string(startColumn));
        }
        get(); get();
        return true;
    }
    return false;
}

void Lexer::lexOperatorOrPunctuation(std::vector<Token> &tokens,
                                     size_t startLine,
                                     size_t startColumn) {
    char c = peek();
    switch (c) {
        case '+':
            if (pos + 1 < src.size() && src[pos + 1] == '+') {
                tokens.push_back({TokenType::PlusPlus, "++", startLine, startColumn});
                get(); get();
            } else {
                tokens.push_back({TokenType::Plus, "+", startLine, startColumn});
                get();
            }
            break;
        case '-':
            if (pos + 1 < src.size() && src[pos + 1] == '-') {
                tokens.push_back({TokenType::MinusMinus, "--", startLine, startColumn});
                get(); get();
            } else {
                tokens.push_back({TokenType::Minus, "-", startLine, startColumn});
                get();
            }
            break;
        case '*': tokens.push_back({TokenType::Star, "*", startLine, startColumn}); get(); break;
        case '/': tokens.push_back({TokenType::Slash, "/", startLine, startColumn}); get(); break;
        case '%': tokens.push_back({TokenType::Percent, "%", startLine, startColumn}); get(); break;
        case '^': tokens.push_back({TokenType::Caret, "^", startLine, startColumn}); get(); break;
        case '?': tokens.push_back({TokenType::Question, "?", startLine, startColumn}); get(); break;
        case '&':
            if (pos + 1 < src.size() && src[pos + 1] == '&') {
                tokens.push_back({TokenType::AmpAmp, "&&", startLine, startColumn});
                get(); get();
            } else {
                tokens.push_back({TokenType::Amp, "&", startLine, startColumn});
                get();
            }
            break;
        case '|':
            if (pos + 1 < src.size() && src[pos + 1] == '|') {
                tokens.push_back({TokenType::PipePipe, "||", startLine, startColumn});
                get(); get();
            } else {
                throw std::runtime_error(
                    "Operador '|' no soportado en linea " + std::to_string(startLine) +
                    ", columna " + std::to_string(startColumn) + ". Usa '||'.");
            }
            break;
        case '!':
            if (pos + 1 < src.size() && src[pos + 1] == '=') {
                tokens.push_back({TokenType::BangEqual, "!=", startLine, startColumn});
                get(); get();
            } else {
                tokens.push_back({TokenType::Bang, "!", startLine, startColumn});
                get();
            }
            break;
        case '=':
            if (pos + 1 < src.size() && src[pos + 1] == '=') {
                tokens.push_back({TokenType::EqualEqual, "==", startLine, startColumn});
                get(); get();
            } else {
                tokens.push_back({TokenType::Equal, "=", startLine, startColumn});
                get();
            }
            break;
        case '<':
            if (pos + 1 < src.size() && src[pos + 1] == '=') {
                tokens.push_back({TokenType::LessEqual, "<=", startLine, startColumn});
                get(); get();
            } else {
                tokens.push_back({TokenType::Less, "<", startLine, startColumn});
                get();
            }
            break;
        case '>':
            if (pos + 1 < src.size() && src[pos + 1] == '=') {
                tokens.push_back({TokenType::GreaterEqual, ">=", startLine, startColumn});
                get(); get();
            } else {
                tokens.push_back({TokenType::Greater, ">", startLine, startColumn});
                get();
            }
            break;
        case '(': tokens.push_back({TokenType::LParen, "(", startLine, startColumn}); get(); break;
        case ')': tokens.push_back({TokenType::RParen, ")", startLine, startColumn}); get(); break;
        case '{': tokens.push_back({TokenType::LBrace, "{", startLine, startColumn}); get(); break;
        case '}': tokens.push_back({TokenType::RBrace, "}", startLine, startColumn}); get(); break;
        case '[': tokens.push_back({TokenType::LBracket, "[", startLine, startColumn}); get(); break;
        case ']': tokens.push_back({TokenType::RBracket, "]", startLine, startColumn}); get(); break;
        case ':': tokens.push_back({TokenType::Colon, ":", startLine, startColumn}); get(); break;
        case ',': tokens.push_back({TokenType::Comma, ",", startLine, startColumn}); get(); break;
        case ';': tokens.push_back({TokenType::Semicolon, ";", startLine, startColumn}); get(); break;
        case '.': tokens.push_back({TokenType::Dot, ".", startLine, startColumn}); get(); break;
        default: {
            unsigned char bad = static_cast<unsigned char>(c);
            std::string repr;
            if (std::isprint(bad)) {
                repr = std::string(1, c);
            } else {
                repr = "byte " + std::to_string(static_cast<int>(bad));
            }
            throw std::runtime_error(
                "Caracter no reconocido '" + repr + "' en linea " + std::to_string(startLine) +
                ", columna " + std::to_string(startColumn));
        }
    }
}

} // namespace aym
