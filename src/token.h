#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
    // Keywords
    KW_INT, KW_BOOL, KW_STRING,
    KW_FUNC, KW_FOR, KW_TO,
    KW_WHILE, KW_IF, KW_ELSE,
    KW_RETURN, KW_PRINT, KW_INPUT,
    KW_NEWLINE, KW_ARRAY, KW_PATTERN,

    // Literals
    INT_LITERAL,
    BOOL_LITERAL,
    STRING_LITERAL,

    // Identifiers
    ID,

    // Operators
    PLUS, MINUS, MUL, DIV, MOD,
    ASSIGN, EQ, NEQ, LT, GT, LEQ, GEQ,
    AND, OR, NOT,

    // Symbols
    LPAREN, RPAREN,
    LBRACE, RBRACE,
    LBRACKET, RBRACKET,
    COMMA, SEMICOLON,
    LT_SYM, GT_SYM,

    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;

    Token(TokenType t, std::string l, int ln)
        : type(t), lexeme(l), line(ln) {}
};

#endif
