#ifndef AYM_LEXER_H
#define AYM_LEXER_H

#include <string>
#include <vector>

namespace aym {

enum class TokenType {
    Identifier,
    String,
    Number,
    Plus,
    Minus,
    Star,
    Slash,
<<<<<<< codex/implementar-operaciones-básicas-en-compilador
    Equal,
    LBrace,
    RBrace,
=======
>>>>>>> main
    LParen,
    RParen,
    Colon,
    Comma,
    Semicolon,
    KeywordPrint,
    KeywordIf,
    KeywordWhile,
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
