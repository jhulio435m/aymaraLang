#include "lexer.h"
#include <cctype>
#include <stdexcept>

namespace aym {

namespace {

char toLowerAscii(unsigned char ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return static_cast<char>(ch);
}

bool hasBytes(const std::string &s, size_t pos, unsigned char b0, unsigned char b1) {
    return pos + 1 < s.size() &&
           static_cast<unsigned char>(s[pos]) == b0 &&
           static_cast<unsigned char>(s[pos + 1]) == b1;
}

bool hasBytes(const std::string &s, size_t pos, unsigned char b0, unsigned char b1, unsigned char b2) {
    return pos + 2 < s.size() &&
           static_cast<unsigned char>(s[pos]) == b0 &&
           static_cast<unsigned char>(s[pos + 1]) == b1 &&
           static_cast<unsigned char>(s[pos + 2]) == b2;
}

std::string normalizeKeyword(const std::string &word) {
    std::string out;
    out.reserve(word.size());

    for (size_t i = 0; i < word.size();) {
        unsigned char ch = static_cast<unsigned char>(word[i]);

        if (ch < 0x80) {
            char lower = toLowerAscii(ch);
            if (lower == '\'' || lower == '`') {
                ++i;
                continue;
            }
            out.push_back(lower);
            ++i;
            continue;
        }

        // Curly apostrophes: U+2019/U+2018
        if (hasBytes(word, i, 0xE2, 0x80, 0x99) || hasBytes(word, i, 0xE2, 0x80, 0x98)) {
            i += 3;
            continue;
        }

        // Common UTF-8 accented latin letters used by the language.
        if (hasBytes(word, i, 0xC3, 0x9C) || hasBytes(word, i, 0xC3, 0xbc) ||
            hasBytes(word, i, 0xC3, 0x9A) || hasBytes(word, i, 0xC3, 0xba)) {
            out.push_back('u');
            i += 2;
            continue;
        }
        if (hasBytes(word, i, 0xC3, 0x91) || hasBytes(word, i, 0xC3, 0xB1)) {
            out.push_back('n');
            i += 2;
            continue;
        }
        if (hasBytes(word, i, 0xC3, 0x81) || hasBytes(word, i, 0xC3, 0xA1)) {
            out.push_back('a');
            i += 2;
            continue;
        }
        if (hasBytes(word, i, 0xC3, 0x89) || hasBytes(word, i, 0xC3, 0xA9)) {
            out.push_back('e');
            i += 2;
            continue;
        }
        if (hasBytes(word, i, 0xC3, 0x8D) || hasBytes(word, i, 0xC3, 0xAD)) {
            out.push_back('i');
            i += 2;
            continue;
        }
        if (hasBytes(word, i, 0xC3, 0x93) || hasBytes(word, i, 0xC3, 0xB3)) {
            out.push_back('o');
            i += 2;
            continue;
        }

        // Keep unknown multibyte bytes untouched to avoid accidental deletion.
        out.push_back(static_cast<char>(ch));
        ++i;
    }

    return out;
}

} // namespace

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

bool Lexer::skipComment(size_t startLine, size_t startColumn) {
    if (peek() != '/' || pos + 1 >= src.size()) {
        return false;
    }
    if (src[pos + 1] == '/') { // line comment
        while (pos < src.size() && get() != '\n');
        return true;
    }
    if (src[pos + 1] == '*') { // block comment
        get(); get();
        while (pos + 1 < src.size() && !(peek() == '*' && src[pos + 1] == '/')) get();
        if (pos + 1 >= src.size()) {
            throw std::runtime_error(
                "Unterminated block comment starting at line " + std::to_string(startLine) +
                ", column " + std::to_string(startColumn));
        }
        get(); get();
        return true;
    }
    return false;
}

void Lexer::lexIdentifierOrKeyword(std::vector<Token> &tokens,
                                   size_t startLine,
                                   size_t startColumn) {
    std::string word;
    while (pos < src.size()) {
        char ch = peek();
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '\'' || (ch & 0x80)) {
            word += get();
        } else {
            break;
        }
    }

    std::string normalized = normalizeKeyword(word);

    if (normalized == "qallta") {
        tokens.push_back({TokenType::KeywordStart, word, startLine, startColumn});
    } else if (normalized == "tukuya") {
        tokens.push_back({TokenType::KeywordEnd, word, startLine, startColumn});
    } else if (normalized == "yatiya") {
        tokens.push_back({TokenType::KeywordDeclare, word, startLine, startColumn});
    } else if (normalized == "qillqa") {
        tokens.push_back({TokenType::KeywordPrint, word, startLine, startColumn});
    } else if (normalized == "kasta") {
        tokens.push_back({TokenType::KeywordClass, word, startLine, startColumn});
    } else if (normalized == "machaqa") {
        tokens.push_back({TokenType::KeywordNew, word, startLine, startColumn});
    } else if (normalized == "aka") {
        tokens.push_back({TokenType::KeywordThis, word, startLine, startColumn});
    } else if (normalized == "jila") {
        tokens.push_back({TokenType::KeywordExtends, word, startLine, startColumn});
    } else if (normalized == "sapa") {
        tokens.push_back({TokenType::KeywordPrivate, word, startLine, startColumn});
    } else if (normalized == "sapakasta") {
        tokens.push_back({TokenType::KeywordStatic, word, startLine, startColumn});
    } else if (normalized == "jilaaka") {
        tokens.push_back({TokenType::KeywordSuper, word, startLine, startColumn});
    } else if (normalized == "ukaxa") {
        tokens.push_back({TokenType::KeywordIf, word, startLine, startColumn});
    } else if (normalized == "maysatxa") {
        tokens.push_back({TokenType::KeywordElse, word, startLine, startColumn});
    } else if (normalized == "ukhakamaxa") {
        tokens.push_back({TokenType::KeywordWhile, word, startLine, startColumn});
    } else if (normalized == "kuti") {
        tokens.push_back({TokenType::KeywordFor, word, startLine, startColumn});
    } else if (normalized == "pakhina") {
        tokens.push_back({TokenType::KeywordBreak, word, startLine, startColumn});
    } else if (normalized == "sarantana") {
        tokens.push_back({TokenType::KeywordContinue, word, startLine, startColumn});
    } else if (normalized == "lurawi") {
        tokens.push_back({TokenType::KeywordFunc, word, startLine, startColumn});
    } else if (normalized == "kuttaya") {
        tokens.push_back({TokenType::KeywordReturn, word, startLine, startColumn});
    } else if (normalized == "apnaq") {
        tokens.push_back({TokenType::KeywordImport, word, startLine, startColumn});
    } else if (normalized == "siqicha") {
        tokens.push_back({TokenType::KeywordEnum, word, startLine, startColumn});
    } else if (normalized == "khiti") {
        tokens.push_back({TokenType::KeywordMatch, word, startLine, startColumn});
    } else if (normalized == "kuna") {
        tokens.push_back({TokenType::KeywordCase, word, startLine, startColumn});
    } else if (normalized == "yaqha") {
        tokens.push_back({TokenType::KeywordDefault, word, startLine, startColumn});
    } else if (normalized == "yantana") {
        tokens.push_back({TokenType::KeywordTry, word, startLine, startColumn});
    } else if (normalized == "katjana") {
        tokens.push_back({TokenType::KeywordCatch, word, startLine, startColumn});
    } else if (normalized == "tukuyawi") {
        tokens.push_back({TokenType::KeywordFinally, word, startLine, startColumn});
    } else if (normalized == "pantja") {
        tokens.push_back({TokenType::KeywordThrow, word, startLine, startColumn});
    } else if (normalized == "jakhuwi") {
        tokens.push_back({TokenType::KeywordTypeNumber, word, startLine, startColumn});
    } else if (normalized == "aru") {
        tokens.push_back({TokenType::KeywordTypeString, word, startLine, startColumn});
    } else if (normalized == "taqa") {
        tokens.push_back({TokenType::KeywordTypeList, word, startLine, startColumn});
    } else if (normalized == "mapa") {
        tokens.push_back({TokenType::KeywordTypeMap, word, startLine, startColumn});
    } else if (normalized == "chiqa") {
        tokens.push_back({TokenType::KeywordTrue, word, startLine, startColumn});
    } else if (normalized == "kari") {
        tokens.push_back({TokenType::KeywordFalse, word, startLine, startColumn});
    } else {
        tokens.push_back({TokenType::Identifier, word, startLine, startColumn});
    }
}

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

void Lexer::lexOperatorOrPunctuation(std::vector<Token> &tokens,
                                     size_t startLine,
                                     size_t startColumn) {
    char c = peek();
    switch (c) {
        case '+':
            if (pos + 1 < src.size() && src[pos + 1] == '+') {
                tokens.push_back({TokenType::PlusPlus, "++", startLine, startColumn});
                get(); get();
            } else {
                tokens.push_back({TokenType::Plus, "+", startLine, startColumn});
                get();
            }
            break;
        case '-':
            if (pos + 1 < src.size() && src[pos + 1] == '-') {
                tokens.push_back({TokenType::MinusMinus, "--", startLine, startColumn});
                get(); get();
            } else {
                tokens.push_back({TokenType::Minus, "-", startLine, startColumn});
                get();
            }
            break;
        case '*': tokens.push_back({TokenType::Star, "*", startLine, startColumn}); get(); break;
        case '/': tokens.push_back({TokenType::Slash, "/", startLine, startColumn}); get(); break;
        case '%': tokens.push_back({TokenType::Percent, "%", startLine, startColumn}); get(); break;
        case '^': tokens.push_back({TokenType::Caret, "^", startLine, startColumn}); get(); break;
        case '?': tokens.push_back({TokenType::Question, "?", startLine, startColumn}); get(); break;
        case '&':
            if (pos + 1 < src.size() && src[pos + 1] == '&') {
                tokens.push_back({TokenType::AmpAmp, "&&", startLine, startColumn});
                get(); get();
            } else {
                tokens.push_back({TokenType::Amp, "&", startLine, startColumn});
                get();
            }
            break;
        case '|':
            if (pos + 1 < src.size() && src[pos + 1] == '|') {
                tokens.push_back({TokenType::PipePipe, "||", startLine, startColumn});
                get(); get();
            } else {
                throw std::runtime_error(
                    "Operador '|' no soportado en linea " + std::to_string(startLine) +
                    ", columna " + std::to_string(startColumn) + ". Usa '||'.");
            }
            break;
        case '!':
            if (pos + 1 < src.size() && src[pos + 1] == '=') {
                tokens.push_back({TokenType::BangEqual, "!=", startLine, startColumn});
                get(); get();
            } else {
                tokens.push_back({TokenType::Bang, "!", startLine, startColumn});
                get();
            }
            break;
        case '=':
            if (pos + 1 < src.size() && src[pos + 1] == '=') {
                tokens.push_back({TokenType::EqualEqual, "==", startLine, startColumn});
                get(); get();
            } else {
                tokens.push_back({TokenType::Equal, "=", startLine, startColumn});
                get();
            }
            break;
        case '<':
            if (pos + 1 < src.size() && src[pos + 1] == '=') {
                tokens.push_back({TokenType::LessEqual, "<=", startLine, startColumn});
                get(); get();
            } else {
                tokens.push_back({TokenType::Less, "<", startLine, startColumn});
                get();
            }
            break;
        case '>':
            if (pos + 1 < src.size() && src[pos + 1] == '=') {
                tokens.push_back({TokenType::GreaterEqual, ">=", startLine, startColumn});
                get(); get();
            } else {
                tokens.push_back({TokenType::Greater, ">", startLine, startColumn});
                get();
            }
            break;
        case '(': tokens.push_back({TokenType::LParen, "(", startLine, startColumn}); get(); break;
        case ')': tokens.push_back({TokenType::RParen, ")", startLine, startColumn}); get(); break;
        case '{': tokens.push_back({TokenType::LBrace, "{", startLine, startColumn}); get(); break;
        case '}': tokens.push_back({TokenType::RBrace, "}", startLine, startColumn}); get(); break;
        case '[': tokens.push_back({TokenType::LBracket, "[", startLine, startColumn}); get(); break;
        case ']': tokens.push_back({TokenType::RBracket, "]", startLine, startColumn}); get(); break;
        case ':': tokens.push_back({TokenType::Colon, ":", startLine, startColumn}); get(); break;
        case ',': tokens.push_back({TokenType::Comma, ",", startLine, startColumn}); get(); break;
        case ';': tokens.push_back({TokenType::Semicolon, ";", startLine, startColumn}); get(); break;
        case '.': tokens.push_back({TokenType::Dot, ".", startLine, startColumn}); get(); break;
        default: {
            unsigned char bad = static_cast<unsigned char>(c);
            std::string repr;
            if (std::isprint(bad)) {
                repr = std::string(1, c);
            } else {
                repr = "byte " + std::to_string(static_cast<int>(bad));
            }
            throw std::runtime_error(
                "Caracter no reconocido '" + repr + "' en linea " + std::to_string(startLine) +
                ", columna " + std::to_string(startColumn));
        }
    }
}

} // namespace aym
