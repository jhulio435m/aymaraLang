#ifndef AYM_LEXER_H
#define AYM_LEXER_H

#include <string>
#include <vector>

namespace aym {

enum class TokenType {
    Identifier,
    String,
    LParen,
    RParen,
    Colon,
    Semicolon,
    Newline,
    KeywordPrint,
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
    char peek() const;
    char get();

    std::string src;
    size_t pos = 0;
};

} // namespace aym

#endif // AYM_LEXER_H
