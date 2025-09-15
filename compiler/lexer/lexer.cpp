#include "lexer.h"
#include <cctype>
#include <stdexcept>

namespace aym {

Lexer::Lexer(const std::string &source) : src(source) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (pos < src.size()) {
        char c = peek();
        if (std::isspace(static_cast<unsigned char>(c))) {
            get();
            continue;
        }

        size_t startLine = line;
        size_t startColumn = column;

        if (c == '/' && pos + 1 < src.size()) {
            if (src[pos + 1] == '/') { // line comment
                while (pos < src.size() && get() != '\n');
                continue;
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
                continue;
            }
        }

        if (std::isalpha(static_cast<unsigned char>(c)) || (c & 0x80)) {
            std::string word;
            while (pos < src.size()) {
                char ch = peek();
                if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || (ch & 0x80)) {
                    word += get();
                } else {
                    break;
                }
            }
            if (word == "willt’aña") {
                tokens.push_back({TokenType::KeywordPrint, word, startLine, startColumn});
            } else if (word == "si") {
                tokens.push_back({TokenType::KeywordIf, word, startLine, startColumn});
            } else if (word == "sino") {
                tokens.push_back({TokenType::KeywordElse, word, startLine, startColumn});
            } else if (word == "mientras") {
                tokens.push_back({TokenType::KeywordWhile, word, startLine, startColumn});
            } else if (word == "haceña") {
                tokens.push_back({TokenType::KeywordDo, word, startLine, startColumn});
            } else if (word == "para") {
                tokens.push_back({TokenType::KeywordFor, word, startLine, startColumn});
            } else if (word == "en") {
                tokens.push_back({TokenType::KeywordIn, word, startLine, startColumn});
            } else if (word == "jalña") {
                tokens.push_back({TokenType::KeywordBreak, word, startLine, startColumn});
            } else if (word == "sarantaña") {
                tokens.push_back({TokenType::KeywordContinue, word, startLine, startColumn});
            } else if (word == "luräwi") {
                tokens.push_back({TokenType::KeywordFunc, word, startLine, startColumn});
            } else if (word == "kutiyana") {
                tokens.push_back({TokenType::KeywordReturn, word, startLine, startColumn});
            } else if (word == "tantachaña") {
                tokens.push_back({TokenType::KeywordSwitch, word, startLine, startColumn});
            } else if (word == "jamusa") {
                tokens.push_back({TokenType::KeywordCase, word, startLine, startColumn});
            } else if (word == "akhamawa") {
                tokens.push_back({TokenType::KeywordDefault, word, startLine, startColumn});
            } else if (word == "uka") {
                tokens.push_back({TokenType::KeywordAnd, word, startLine, startColumn});
            } else if (word == "jan") {
                size_t i = pos;
                while (i < src.size() && std::isspace(static_cast<unsigned char>(src[i]))) ++i;
                std::string next;
                while (i < src.size()) {
                    char ch2 = src[i];
                    if (std::isalnum(static_cast<unsigned char>(ch2)) || ch2 == '_' || (ch2 & 0x80)) {
                        next += ch2;
                        ++i;
                    } else {
                        break;
                    }
                }
                if (next == "uka") {
                    while (pos < src.size() && std::isspace(static_cast<unsigned char>(peek()))) get();
                    for (size_t j = 0; j < 3 && pos < src.size(); ++j) get();
                    tokens.push_back({TokenType::KeywordOr, "jan uka", startLine, startColumn});
                } else if (next == "cheka") {
                    while (pos < src.size() && std::isspace(static_cast<unsigned char>(peek()))) get();
                    for (size_t j = 0; j < 5 && pos < src.size(); ++j) get();
                    tokens.push_back({TokenType::Number, "0", startLine, startColumn});
                } else {
                    tokens.push_back({TokenType::Identifier, word, startLine, startColumn});
                }
            } else if (word == "janiwa") {
                tokens.push_back({TokenType::KeywordNot, word, startLine, startColumn});
            } else if (word == "jach’a") {
                tokens.push_back({TokenType::KeywordInt, word, startLine, startColumn});
            } else if (word == "lliphiphi") {
                tokens.push_back({TokenType::KeywordFloat, word, startLine, startColumn});
            } else if (word == "chuymani") {
                tokens.push_back({TokenType::KeywordBool, word, startLine, startColumn});
            } else if (word == "qillqa") {
                tokens.push_back({TokenType::KeywordString, word, startLine, startColumn});
            } else if (word == "cheka") {
                tokens.push_back({TokenType::Number, "1", startLine, startColumn});
            } else {
                tokens.push_back({TokenType::Identifier, word, startLine, startColumn});
            }
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            std::string num;
            while (pos < src.size() && std::isdigit(static_cast<unsigned char>(peek()))) {
                num += get();
            }
            tokens.push_back({TokenType::Number, num, startLine, startColumn});
            continue;
        }

        if (c == '"') {
            get();
            std::string str;
            bool terminated = false;
            while (pos < src.size()) {
                char ch = get();
                if (ch == '"') { terminated = true; break; }
                if (ch == '\\') {
                    if (pos >= src.size()) break;
                    char esc = get();
                    switch (esc) {
                        case 'n': str += '\n'; break;
                        case 't': str += '\t'; break;
                        case 'r': str += '\r'; break;
                        case '\\': str += '\\'; break;
                        case '"': str += '"'; break;
                        case '0': str += '\0'; break;
                        default: str += esc; break;
                    }
                } else {
                    str += ch;
                }
            }
            if (!terminated) {
                throw std::runtime_error(
                    "Unterminated string starting at line " + std::to_string(startLine) +
                    ", column " + std::to_string(startColumn));
            }
            tokens.push_back({TokenType::String, str, startLine, startColumn});
            continue;
        }

        switch (c) {
            case '+': tokens.push_back({TokenType::Plus, "+", startLine, startColumn}); get(); break;
            case '-': tokens.push_back({TokenType::Minus, "-", startLine, startColumn}); get(); break;
            case '*': tokens.push_back({TokenType::Star, "*", startLine, startColumn}); get(); break;
            case '/': tokens.push_back({TokenType::Slash, "/", startLine, startColumn}); get(); break;
            case '%': tokens.push_back({TokenType::Percent, "%", startLine, startColumn}); get(); break;
            case '^': tokens.push_back({TokenType::Caret, "^", startLine, startColumn}); get(); break;
            case '&':
                if (pos + 1 < src.size() && src[pos + 1] == '&') {
                    tokens.push_back({TokenType::AmpAmp, "&&", startLine, startColumn});
                    get(); get();
                } else {
                    get();
                }
                break;
            case '|':
                if (pos + 1 < src.size() && src[pos + 1] == '|') {
                    tokens.push_back({TokenType::PipePipe, "||", startLine, startColumn});
                    get(); get();
                } else {
                    get();
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
            default: get(); break;
        }
    }

    tokens.push_back({TokenType::EndOfFile, "", line, column});
    return tokens;
}

char Lexer::peek() const {
    return pos < src.size() ? src[pos] : '\0';
}

char Lexer::get() {
    if (pos < src.size()) {
        char c = src[pos++];
        if (c == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
        return c;
    }
    return '\0';
}

} // namespace aym
