#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "token.h"

class Lexer {
private:
    std::string source;
    size_t pos = 0;
    int line = 1;

    char peek();
    char advance();
    bool match(char expected);
    bool isAtEnd();
    void skipWhitespace();
    void skipComment();

    Token identifier();
    Token number();
    Token stringLiteral();

    Token makeToken(TokenType type, std::string lex = "");
public:
    Lexer(const std::string &src);
    std::vector<Token> tokenize();
};

#endif
