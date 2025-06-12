#ifndef AYM_LEXER_H
#define AYM_LEXER_H

#include <string>
#include <vector>

namespace aym {

enum class TokenType {
    Identifier,
    Number,
    String,
    EndOfFile
};

struct Token {
    TokenType type;
    std::string text;
};

class Lexer {
public:
    explicit Lexer(const std::string &source);
    std::vector<Token> tokenize();
private:
    std::string src;
};

} // namespace aym

#endif // AYM_LEXER_H
