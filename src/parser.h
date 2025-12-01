#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include "token.h"
#include "ast.h"

class Parser {
    std::vector<Token> tokens;
    int current = 0;

    Token peek();
    Token advance();
    bool match(TokenType t);
    bool check(TokenType t);
    bool isAtEnd();

    void expect(TokenType t, const std::string &msg);

    // Parsing functions (matching grammar)
    Program* parseProgram();
    ASTNode* declaration();
    FuncDecl* funcDeclaration();
    VarDeclStmt* varDeclaration();

    BlockStmt* block();
    Stmt* statement();
    Stmt* forStmt();
    Stmt* whileStmt();
    Stmt* ifStmt();
    Stmt* printStmt();
    Stmt* returnStmt();
    Stmt* inputStmt();
    Stmt* newlineStmt();
    Stmt* assignStmt();

    // Expression parsing
    Expr* expr();
    Expr* logic_or();
    Expr* logic_and();
    Expr* equality();
    Expr* rel();
    Expr* add();
    Expr* mul();
    Expr* unary();
    Expr* primary();

public:
    Parser(const std::vector<Token> &toks);
    Program* parse();
};

#endif
