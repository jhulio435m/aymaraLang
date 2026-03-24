#include "lexer.h"
#include <cctype>

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
        if (hasBytes(word, i, 0xC3, 0x9C) || hasBytes(word, i, 0xC3, 0xBC) ||
            hasBytes(word, i, 0xC3, 0x9A) || hasBytes(word, i, 0xC3, 0xBA)) {
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


} // namespace aym
