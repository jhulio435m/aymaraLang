#include "lexer.h"
#include <cctype>
#include <stdexcept>

namespace aym {

void Lexer::lexNumber(std::vector<Token> &tokens,
                      size_t startLine,
                      size_t startColumn) {
    std::string num;
    char c = peek();
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
            return;
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
            return;
        }
    }

    while (pos < src.size() && std::isdigit(static_cast<unsigned char>(peek()))) {
        num += get();
    }
    tokens.push_back({TokenType::Number, num, startLine, startColumn});
}


void Lexer::lexInterpolatedString(std::vector<Token> &tokens,
                                  size_t startLine,
                                  size_t startColumn) {
    get(); // consume $
    char quote = get();
    std::string str;
    bool terminated = false;
    while (pos < src.size()) {
        char ch = get();
        if (ch == quote) {
            terminated = true;
            break;
        }
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
}


void Lexer::lexString(std::vector<Token> &tokens,
                      size_t startLine,
                      size_t startColumn) {
    char quote = get();
    std::string str;
    bool terminated = false;
    while (pos < src.size()) {
        char ch = get();
        if (ch == quote) {
            terminated = true;
            break;
        }
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
}


} // namespace aym
