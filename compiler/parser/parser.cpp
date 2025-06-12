#include "parser.h"

namespace aym {

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

void Parser::parse() {
    for (const auto& token : tokens) {
        std::cout << "TOKEN: " << token.text << std::endl;
    }
}

} // namespace aym
