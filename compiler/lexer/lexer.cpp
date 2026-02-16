#include "lexer.h"

namespace aym {

Lexer::Lexer(const std::string &source) : src(source) {}

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
