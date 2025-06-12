#ifndef AYM_PARSER_H
#define AYM_PARSER_H

#include <vector>
#include <iostream>
#include "../lexer/lexer.h"

namespace aym {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    void parse();
private:
    std::vector<Token> tokens;
};

} // namespace aym

#endif // AYM_PARSER_H
