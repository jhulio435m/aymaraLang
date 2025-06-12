#include "lexer.h"
#include <sstream>

namespace aym {

Lexer::Lexer(const std::string &source) : src(source) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    std::istringstream iss(src);
    std::string word;
    while (iss >> word) {
        tokens.push_back({TokenType::Identifier, word});
    }
    tokens.push_back({TokenType::EndOfFile, ""});
    return tokens;
}

} // namespace aym
