#include "lexer.h"
#include <cctype>

namespace aym {

Lexer::Lexer(const std::string &source) : src(source) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (pos < src.size()) {
        char c = peek();
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (c == '\n') {
                tokens.push_back({TokenType::Newline, "\n"});
            }
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
            } else {
                tokens.push_back({TokenType::Identifier, word});
            }
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
            case '(': tokens.push_back({TokenType::LParen, "("}); get(); break;
            case ')': tokens.push_back({TokenType::RParen, ")"}); get(); break;
            case ':': tokens.push_back({TokenType::Colon, ":"}); get(); break;
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
