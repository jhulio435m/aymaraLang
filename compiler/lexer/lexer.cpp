#include "lexer.h"
#include <cctype>

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

        if (c == '/' && pos + 1 < src.size()) {
            if (src[pos + 1] == '/') { // line comment
                while (pos < src.size() && get() != '\n');
                continue;
            }
            if (src[pos + 1] == '*') { // block comment
                get(); get();
                while (pos + 1 < src.size() && !(peek() == '*' && src[pos + 1] == '/')) get();
                if (pos + 1 < src.size()) { get(); get(); }
                continue;
            }
        }

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '\xc3' || c == '\xE2') {
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
                tokens.push_back({TokenType::KeywordPrint, word});
            } else if (word == "si") {
                tokens.push_back({TokenType::KeywordIf, word});
            } else if (word == "sino") {
                tokens.push_back({TokenType::KeywordElse, word});
            } else if (word == "mientras") {
                tokens.push_back({TokenType::KeywordWhile, word});
            } else if (word == "para") {
                tokens.push_back({TokenType::KeywordFor, word});
            } else if (word == "break") {
                tokens.push_back({TokenType::KeywordBreak, word});
            } else if (word == "continue") {
                tokens.push_back({TokenType::KeywordContinue, word});
            } else if (word == "func") {
                tokens.push_back({TokenType::KeywordFunc, word});
            } else if (word == "retorna") {
                tokens.push_back({TokenType::KeywordReturn, word});
            } else if (word == "switch") {
                tokens.push_back({TokenType::KeywordSwitch, word});
            } else if (word == "case") {
                tokens.push_back({TokenType::KeywordCase, word});
            } else if (word == "default") {
                tokens.push_back({TokenType::KeywordDefault, word});
            } else if (word == "and") {
                tokens.push_back({TokenType::KeywordAnd, word});
            } else if (word == "or") {
                tokens.push_back({TokenType::KeywordOr, word});
            } else if (word == "not") {
                tokens.push_back({TokenType::KeywordNot, word});
            } else if (word == "int") {
                tokens.push_back({TokenType::KeywordInt, word});
            } else if (word == "float") {
                tokens.push_back({TokenType::KeywordFloat, word});
            } else if (word == "bool") {
                tokens.push_back({TokenType::KeywordBool, word});
            } else if (word == "string") {
                tokens.push_back({TokenType::KeywordString, word});
            } else if (word == "true") {
                tokens.push_back({TokenType::Number, "1"});
            } else if (word == "false") {
                tokens.push_back({TokenType::Number, "0"});
            } else {
                tokens.push_back({TokenType::Identifier, word});
            }
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            std::string num;
            while (pos < src.size() && std::isdigit(static_cast<unsigned char>(peek()))) {
                num += get();
            }
            tokens.push_back({TokenType::Number, num});
            continue;
        }

        if (c == '"') {
            get();
            std::string str;
            while (pos < src.size() && peek() != '"') {
                str += get();
            }
            get(); // closing quote
            tokens.push_back({TokenType::String, str});
            continue;
        }

        switch (c) {
            case '+': tokens.push_back({TokenType::Plus, "+"}); get(); break;
            case '-': tokens.push_back({TokenType::Minus, "-"}); get(); break;
            case '*': tokens.push_back({TokenType::Star, "*"}); get(); break;
            case '/': tokens.push_back({TokenType::Slash, "/"}); get(); break;
            case '%': tokens.push_back({TokenType::Percent, "%"}); get(); break;
            case '^': tokens.push_back({TokenType::Caret, "^"}); get(); break;
            case '&':
                if (pos + 1 < src.size() && src[pos + 1] == '&') {
                    tokens.push_back({TokenType::AmpAmp, "&&"});
                    get(); get();
                } else {
                    get();
                }
                break;
            case '|':
                if (pos + 1 < src.size() && src[pos + 1] == '|') {
                    tokens.push_back({TokenType::PipePipe, "||"});
                    get(); get();
                } else {
                    get();
                }
                break;
            case '!':
                if (pos + 1 < src.size() && src[pos + 1] == '=') {
                    tokens.push_back({TokenType::BangEqual, "!="});
                    get(); get();
                } else {
                    tokens.push_back({TokenType::Bang, "!"});
                    get();
                }
                break;
            case '=':
                if (pos + 1 < src.size() && src[pos + 1] == '=') {
                    tokens.push_back({TokenType::EqualEqual, "=="});
                    get(); get();
                } else {
                    tokens.push_back({TokenType::Equal, "="});
                    get();
                }
                break;
            case '<':
                if (pos + 1 < src.size() && src[pos + 1] == '=') {
                    tokens.push_back({TokenType::LessEqual, "<="});
                    get(); get();
                } else {
                    tokens.push_back({TokenType::Less, "<"});
                    get();
                }
                break;
            case '>':
                if (pos + 1 < src.size() && src[pos + 1] == '=') {
                    tokens.push_back({TokenType::GreaterEqual, ">="});
                    get(); get();
                } else {
                    tokens.push_back({TokenType::Greater, ">"});
                    get();
                }
                break;
            case '(': tokens.push_back({TokenType::LParen, "("}); get(); break;
            case ')': tokens.push_back({TokenType::RParen, ")"}); get(); break;
            case '{': tokens.push_back({TokenType::LBrace, "{"}); get(); break;
            case '}': tokens.push_back({TokenType::RBrace, "}"}); get(); break;
            case '[': tokens.push_back({TokenType::LBracket, "["}); get(); break;
            case ']': tokens.push_back({TokenType::RBracket, "]"}); get(); break;
            case ':': tokens.push_back({TokenType::Colon, ":"}); get(); break;
            case ',': tokens.push_back({TokenType::Comma, ","}); get(); break;
            case ';': tokens.push_back({TokenType::Semicolon, ";"}); get(); break;
            default: get(); break;
        }
    }

    tokens.push_back({TokenType::EndOfFile, ""});
    return tokens;
}

char Lexer::peek() const {
    return pos < src.size() ? src[pos] : '\0';
}

char Lexer::get() {
    if (pos < src.size()) return src[pos++];
    return '\0';
}

} // namespace aym
