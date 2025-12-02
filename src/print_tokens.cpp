#include "print_tokens.h"
#include <iostream>
#include <unordered_map>

static std::string tokenTypeToString(TokenType t) {
    switch (t) {
        case TokenType::KW_INT: return "KW_INT";
        case TokenType::KW_BOOL: return "KW_BOOL";
        case TokenType::KW_STRING: return "KW_STRING";
        case TokenType::KW_FUNC: return "KW_FUNC";
        case TokenType::KW_FOR: return "KW_FOR";
        case TokenType::KW_TO: return "KW_TO";
        case TokenType::KW_WHILE: return "KW_WHILE";
        case TokenType::KW_IF: return "KW_IF";
        case TokenType::KW_ELSE: return "KW_ELSE";
        case TokenType::KW_RETURN: return "KW_RETURN";
        case TokenType::KW_PRINT: return "KW_PRINT";
        case TokenType::KW_INPUT: return "KW_INPUT";
        case TokenType::KW_NEWLINE: return "KW_NEWLINE";
        case TokenType::KW_PATTERN: return "KW_PATTERN";

        case TokenType::INT_LITERAL: return "INT_LITERAL";
        case TokenType::BOOL_LITERAL: return "BOOL_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";

        case TokenType::ID: return "ID";

        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MUL: return "MUL";
        case TokenType::DIV: return "DIV";
        case TokenType::MOD: return "MOD";

        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::EQ: return "EQ";
        case TokenType::NEQ: return "NEQ";
        case TokenType::LT: return "LT";
        case TokenType::GT: return "GT";
        case TokenType::LEQ: return "LEQ";
        case TokenType::GEQ: return "GEQ";

        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";

        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";

        case TokenType::COMMA: return "COMMA";
        case TokenType::SEMICOLON: return "SEMICOLON";

        case TokenType::END_OF_FILE: return "END_OF_FILE";
        default: return "UNKNOWN";
    }
}

void printTokens(const std::vector<Token>& tokens) {
    std::cout << "\n=== TOKENS ===\n";
    for (const auto& t : tokens) {
        std::cout << "Token(type='" << tokenTypeToString(t.type)
                  << "', val='" << t.lexeme
                  << "', line=" << t.line
                  << ")\n";
    }
    std::cout << "=== END TOKENS ===\n\n";
}
