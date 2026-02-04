#ifndef AYM_LEXER_H
#define AYM_LEXER_H

#include <string>
#include <vector>

namespace aym {

enum class TokenType {
    Identifier,
    String,
    InterpolatedString,
    Number,
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Caret,
    Question,
    PlusPlus,
    MinusMinus,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    EqualEqual,
    BangEqual,
    Bang,
    AmpAmp,
    PipePipe,
    Equal,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    LParen,
    RParen,
    Colon,
    Comma,
    Semicolon,
    Dot,
    KeywordStart,
    KeywordEnd,
    KeywordDeclare,
    KeywordPrint,
    KeywordIf,
    KeywordElse,
    KeywordWhile,
    KeywordFor,
    KeywordBreak,
    KeywordContinue,
    KeywordFunc,
    KeywordReturn,
    KeywordImport,
    KeywordTypeNumber,
    KeywordTypeString,
    KeywordTypeBool,
    KeywordTypeList,
    KeywordTypeMap,
    KeywordTrue,
    KeywordFalse,
    EndOfFile
};

struct Token {
    TokenType type;
    std::string text;
    size_t line;
    size_t column;
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
    size_t line = 1;
    size_t column = 1;
};

} // namespace aym

#endif // AYM_LEXER_H
