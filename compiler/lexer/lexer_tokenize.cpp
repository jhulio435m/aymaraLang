#include "lexer.h"
#include <cctype>

namespace aym {

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (pos < src.size()) {
        if (std::isspace(static_cast<unsigned char>(peek()))) {
            get();
            continue;
        }

        size_t startLine = line;
        size_t startColumn = column;

        if (skipComment(startLine, startColumn)) {
            continue;
        }

        char c = peek();
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' || (c & 0x80)) {
            lexIdentifierOrKeyword(tokens, startLine, startColumn);
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(c))) {
            lexNumber(tokens, startLine, startColumn);
            continue;
        }
        if (c == '$' && pos + 1 < src.size() && src[pos + 1] == '"') {
            lexInterpolatedString(tokens, startLine, startColumn);
            continue;
        }
        if (c == '"' || c == '\'') {
            lexString(tokens, startLine, startColumn);
            continue;
        }

        lexOperatorOrPunctuation(tokens, startLine, startColumn);
    }

    tokens.push_back({TokenType::EndOfFile, "", line, column});
    return tokens;
}

} // namespace aym
