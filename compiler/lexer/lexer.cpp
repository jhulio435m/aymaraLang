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
            } else if (word == "int") {
                tokens.push_back({TokenType::KeywordInt, word});
            } else if (word == "float") {
                tokens.push_back({TokenType::KeywordFloat, word});
            } else if (word == "bool") {
                tokens.push_back({TokenType::KeywordBool, word});
            } else if (word == "string") {
                tokens.push_back({TokenType::KeywordString, word});
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
            case '=': tokens.push_back({TokenType::Equal, "="}); get(); break;
            case '(': tokens.push_back({TokenType::LParen, "("}); get(); break;
            case ')': tokens.push_back({TokenType::RParen, ")"}); get(); break;
            case '{': tokens.push_back({TokenType::LBrace, "{"}); get(); break;
            case '}': tokens.push_back({TokenType::RBrace, "}"}); get(); break;
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
