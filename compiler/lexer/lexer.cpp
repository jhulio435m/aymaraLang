#include "lexer.h"
#include <cctype>
#include <stdexcept>
#include <algorithm>

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
                if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '\'' || (ch & 0x80)) {
                    word += get();
                } else {
                    break;
                }
            }
            std::string lower = word;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (lower == "qallta") {
                tokens.push_back({TokenType::KeywordStart, word, startLine, startColumn});
            } else if (lower == "tukuya") {
                tokens.push_back({TokenType::KeywordEnd, word, startLine, startColumn});
            } else if (lower == "yatiya") {
                tokens.push_back({TokenType::KeywordDeclare, word, startLine, startColumn});
            } else if (lower == "qillqa") {
                tokens.push_back({TokenType::KeywordPrint, word, startLine, startColumn});
            } else if (lower == "suti" || lower == "jisa") {
                tokens.push_back({TokenType::KeywordIf, word, startLine, startColumn});
            } else if (lower == "jani" || lower == "maysatxa") {
                tokens.push_back({TokenType::KeywordElse, word, startLine, startColumn});
            } else if (lower == "kunawsati" || lower == "ukhakamaxa") {
                tokens.push_back({TokenType::KeywordWhile, word, startLine, startColumn});
            } else if (lower == "sapüru" || lower == "taki") {
                tokens.push_back({TokenType::KeywordFor, word, startLine, startColumn});
            } else if (lower == "p'akhiña") {
                tokens.push_back({TokenType::KeywordBreak, word, startLine, startColumn});
            } else if (lower == "sarantaña") {
                tokens.push_back({TokenType::KeywordContinue, word, startLine, startColumn});
            } else if (lower == "lurawi") {
                tokens.push_back({TokenType::KeywordFunc, word, startLine, startColumn});
            } else if (lower == "kuttaya") {
                tokens.push_back({TokenType::KeywordReturn, word, startLine, startColumn});
            } else if (lower == "apnaq") {
                tokens.push_back({TokenType::KeywordImport, word, startLine, startColumn});
            } else if (lower == "jakhüwi") {
                tokens.push_back({TokenType::KeywordTypeNumber, word, startLine, startColumn});
            } else if (lower == "aru") {
                tokens.push_back({TokenType::KeywordTypeString, word, startLine, startColumn});
            } else if (lower == "listaña" || lower == "t'aqa") {
                tokens.push_back({TokenType::KeywordTypeList, word, startLine, startColumn});
            } else if (lower == "mapa") {
                tokens.push_back({TokenType::KeywordTypeMap, word, startLine, startColumn});
            } else if (lower == "utji" || lower == "chiqa") {
                tokens.push_back({TokenType::KeywordTrue, word, startLine, startColumn});
            } else if (lower == "janiutji" || lower == "k'ari") {
                tokens.push_back({TokenType::KeywordFalse, word, startLine, startColumn});
            } else {
                tokens.push_back({TokenType::Identifier, word, startLine, startColumn});
            }
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            std::string num;
            if (c == '0' && pos + 1 < src.size()) {
                char next = src[pos + 1];
                if (next == 'x' || next == 'X') {
                    num += get();
                    num += get();
                    while (pos < src.size() && std::isxdigit(static_cast<unsigned char>(peek()))) {
                        num += get();
                    }
                    if (num.size() <= 2) {
                        throw std::runtime_error(
                            "Literal hexadecimal incompleto en linea " + std::to_string(startLine) +
                            ", columna " + std::to_string(startColumn));
                    }
                    tokens.push_back({TokenType::Number, num, startLine, startColumn});
                    continue;
                }
                if (next == 'b' || next == 'B') {
                    num += get();
                    num += get();
                    while (pos < src.size()) {
                        char digit = peek();
                        if (digit == '0' || digit == '1') {
                            num += get();
                        } else {
                            break;
                        }
                    }
                    if (num.size() <= 2) {
                        throw std::runtime_error(
                            "Literal binario incompleto en linea " + std::to_string(startLine) +
                            ", columna " + std::to_string(startColumn));
                    }
                    tokens.push_back({TokenType::Number, num, startLine, startColumn});
                    continue;
                }
            }
            while (pos < src.size() && std::isdigit(static_cast<unsigned char>(peek()))) {
                num += get();
            }
            tokens.push_back({TokenType::Number, num, startLine, startColumn});
            continue;
        }

        if (c == '$' && pos + 1 < src.size() && src[pos + 1] == '"') {
            get(); // consume $
            char quote = get();
            std::string str;
            bool terminated = false;
            while (pos < src.size()) {
                char ch = get();
                if (ch == quote) { terminated = true; break; }
                if (ch == '\\') {
                    if (pos >= src.size()) break;
                    char esc = get();
                    switch (esc) {
                        case 'n': str += '\n'; break;
                        case 't': str += '\t'; break;
                        case 'r': str += '\r'; break;
                        case '\\': str += '\\'; break;
                        case '"': str += '"'; break;
                        case '\'': str += '\''; break;
                        case '{': str += '{'; break;
                        case '}': str += '}'; break;
                        case '0': str += '\0'; break;
                        default: str += esc; break;
                    }
                } else {
                    str += ch;
                }
            }
            if (!terminated) {
                throw std::runtime_error(
                    "Unterminated interpolated string starting at line " + std::to_string(startLine) +
                    ", column " + std::to_string(startColumn));
            }
            tokens.push_back({TokenType::InterpolatedString, str, startLine, startColumn});
            continue;
        }

        if (c == '"' || c == '\'') {
            char quote = get();
            std::string str;
            bool terminated = false;
            while (pos < src.size()) {
                char ch = get();
                if (ch == quote) { terminated = true; break; }
                if (ch == '\\') {
                    if (pos >= src.size()) break;
                    char esc = get();
                    switch (esc) {
                        case 'n': str += '\n'; break;
                        case 't': str += '\t'; break;
                        case 'r': str += '\r'; break;
                        case '\\': str += '\\'; break;
                        case '"': str += '"'; break;
                        case '\'': str += '\''; break;
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
            case '.': tokens.push_back({TokenType::Dot, ".", startLine, startColumn}); get(); break;
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
