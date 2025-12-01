#include "lexer.h"
#include <cctype>
#include <iostream>

Lexer::Lexer(const std::string &src) : source(src) {}

char Lexer::peek() {
    return isAtEnd() ? '\0' : source[pos];
}

char Lexer::advance() {
    return isAtEnd() ? '\0' : source[pos++];
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source[pos] != expected) return false;
    pos++;
    return true;
}

bool Lexer::isAtEnd() {
    return pos >= source.size();
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') advance();
        else if (c == '\n') { line++; advance(); }
        else break;
    }
}

void Lexer::skipComment() {
    if (match('/') && match('/')) {
        while (!isAtEnd() && peek() != '\n') advance();
    }
    else if (source[pos-1] == '/' && match('*')) {
        while (!isAtEnd()) {
            if (peek() == '*' && source[pos+1] == '/') {
                advance(); advance();
                break;
            }
            if (peek() == '\n') line++;
            advance();
        }
    }
}

Token Lexer::makeToken(TokenType type, std::string lex) {
    return Token(type, lex, line);
}

Token Lexer::identifier() {
    size_t start = pos - 1;
    while (std::isalnum(peek()) || peek() == '_') advance();
    std::string text = source.substr(start, pos - start);

    if (text == "int") return makeToken(TokenType::KW_INT, text);
    if (text == "bool") return makeToken(TokenType::KW_BOOL, text);
    if (text == "string") return makeToken(TokenType::KW_STRING, text);
    if (text == "func") return makeToken(TokenType::KW_FUNC, text);
    if (text == "for") return makeToken(TokenType::KW_FOR, text);
    if (text == "to") return makeToken(TokenType::KW_TO, text);
    if (text == "while") return makeToken(TokenType::KW_WHILE, text);
    if (text == "if") return makeToken(TokenType::KW_IF, text);
    if (text == "else") return makeToken(TokenType::KW_ELSE, text);
    if (text == "return") return makeToken(TokenType::KW_RETURN, text);
    if (text == "print") return makeToken(TokenType::KW_PRINT, text);
    if (text == "input") return makeToken(TokenType::KW_INPUT, text);
    if (text == "newline") return makeToken(TokenType::KW_NEWLINE, text);
    if (text == "pattern") return makeToken(TokenType::KW_PATTERN, text);
    if (text == "true" || text == "false")
        return makeToken(TokenType::BOOL_LITERAL, text);

    return makeToken(TokenType::ID, text);
}

Token Lexer::number() {
    size_t start = pos - 1;
    while (isdigit(peek())) advance();
    return makeToken(TokenType::INT_LITERAL,
        source.substr(start, pos - start));
}

Token Lexer::stringLiteral() {
    size_t start = pos;
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\\') advance();
        advance();
    }
    advance(); // closing "
    return makeToken(TokenType::STRING_LITERAL,
        source.substr(start, pos - start - 1));
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespace();
        
        if(isAtEnd()) break;

        char c = advance();

        if (c == '/') {
            if (peek() == '/' || peek() == '*') {
                skipComment();
                continue;
            }
        }

        if (std::isalpha(c) || c == '_') {
            tokens.push_back(identifier());
            continue;
        }

        if (isdigit(c)) {
            tokens.push_back(number());
            continue;
        }

        if (c == '"') {
            tokens.push_back(stringLiteral());
            continue;
        }

        // Single or multi-char operators
        switch (c) {
            case '+': tokens.push_back(makeToken(TokenType::PLUS, "+")); break;
            case '-': tokens.push_back(makeToken(TokenType::MINUS, "-")); break;
            case '*': tokens.push_back(makeToken(TokenType::MUL, "*")); break;
            case '%': tokens.push_back(makeToken(TokenType::MOD, "%")); break;
            case '/': tokens.push_back(makeToken(TokenType::DIV, "/")); break;

            case '=':
                if (match('=')) tokens.push_back(makeToken(TokenType::EQ, "=="));
                else tokens.push_back(makeToken(TokenType::ASSIGN, "="));
                break;

            case '!':
                if (match('=')) tokens.push_back(makeToken(TokenType::NEQ, "!="));
                else tokens.push_back(makeToken(TokenType::NOT, "!"));
                break;

            case '<':
                if (match('=')) tokens.push_back(makeToken(TokenType::LEQ, "<="));
                else tokens.push_back(makeToken(TokenType::LT, "<"));
                break;

            case '>':
                if (match('=')) tokens.push_back(makeToken(TokenType::GEQ, ">="));
                else tokens.push_back(makeToken(TokenType::GT, ">"));
                break;

            case '&':
                if (match('&')) tokens.push_back(makeToken(TokenType::AND, "&&"));
                else tokens.push_back(makeToken(TokenType::UNKNOWN, "&"));
                break;

            case '|':
                if (match('|')) tokens.push_back(makeToken(TokenType::OR, "||"));
                else tokens.push_back(makeToken(TokenType::UNKNOWN, "|"));
                break;

            case '(': tokens.push_back(makeToken(TokenType::LPAREN, "(")); break;
            case ')': tokens.push_back(makeToken(TokenType::RPAREN, ")")); break;
            case '{': tokens.push_back(makeToken(TokenType::LBRACE, "{")); break;
            case '}': tokens.push_back(makeToken(TokenType::RBRACE, "}")); break;
            case ',': tokens.push_back(makeToken(TokenType::COMMA, ",")); break;
            case ';': tokens.push_back(makeToken(TokenType::SEMICOLON, ";")); break;

            default:
                tokens.push_back(makeToken(TokenType::UNKNOWN, std::string(1, c)));

        }
    }

    tokens.push_back(makeToken(TokenType::END_OF_FILE, "EOF"));
    return tokens;
}