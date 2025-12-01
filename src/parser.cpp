#include "parser.h"
#include <iostream>

// ==========================
// Helper functions
// ==========================

Parser::Parser(const std::vector<Token> &toks) : tokens(toks) {}

Token Parser::peek() {
    return tokens[current];
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return tokens[current - 1];
}

bool Parser::isAtEnd() {
    return peek().type == TokenType::END_OF_FILE;
}

bool Parser::check(TokenType t) {
    if (isAtEnd()) return false;
    return peek().type == t;
}

bool Parser::match(TokenType t) {
    if (check(t)) {
        advance();
        return true;
    }
    return false;
}

void Parser::expect(TokenType t, const std::string &msg) {
    if (!match(t)) {
        std::cerr << "Parser error at line " << peek().line << ": " << msg << "\n";
        exit(1);
    }
}

// =============================
// Top-level parse
// =============================

Program* Parser::parse() {
    return parseProgram();
}

Program* Parser::parseProgram() {
    Program* prog = new Program();

    while (!isAtEnd()) {
        prog->decls.emplace_back(declaration());
    }

    return prog;
}

// =============================
// Declarations
// =============================

ASTNode* Parser::declaration() {
    if (match(TokenType::KW_FUNC)) {
        return funcDeclaration();
    }
    return varDeclaration();
}

VarDeclStmt* Parser::varDeclaration() {
    // Ensure the current token is a type keyword before consuming it
    if (!(check(TokenType::KW_INT) ||
          check(TokenType::KW_BOOL) ||
          check(TokenType::KW_STRING))) {
        // Helpful debug message showing what we actually saw
        Token p = peek();
        std::cerr << "Parser error at line " << p.line
                  << ": Expected type in variable declaration but found token '"
                  << p.lexeme << "' (type=" << (int)p.type << ")\n";
        exit(1);
    }

    // Now consume the type token
    Token typeTok = advance(); // int / bool / string

    // Next must be identifier
    Token nameTok = peek();
    expect(TokenType::ID, "Expected identifier in variable declaration");

    std::unique_ptr<Expr> init = nullptr;
    if (match(TokenType::ASSIGN)) {
        init.reset(expr());
    }

    expect(TokenType::SEMICOLON, "Expected ';' after variable declaration");

    VarDeclStmt* decl = new VarDeclStmt();
    decl->type = typeTok.lexeme;
    decl->name = nameTok.lexeme;
    decl->init = std::move(init);

    return decl;
}


FuncDecl* Parser::funcDeclaration() {
    Token nameTok = peek();
    expect(TokenType::ID, "Expected function name");

    expect(TokenType::LPAREN, "Expected '(' after function name");

    std::vector<FuncParam> params;

    if (!check(TokenType::RPAREN)) {
        do {
            Token typeTok = advance();
            if (!(typeTok.type == TokenType::KW_INT ||
                  typeTok.type == TokenType::KW_BOOL ||
                  typeTok.type == TokenType::KW_STRING)) {
                std::cerr << "Expected parameter type.\n";
                exit(1);
            }

            Token nameTok = peek();
            expect(TokenType::ID, "Expected parameter name");

            params.push_back({ typeTok.lexeme, nameTok.lexeme });
        }
        while (match(TokenType::COMMA));
    }

    expect(TokenType::RPAREN, "Expected ')' after parameters");

    BlockStmt* body = block();

    FuncDecl* func = new FuncDecl();
    func->name = nameTok.lexeme;
    func->params = params;
    func->body = std::unique_ptr<BlockStmt>(body);

    return func;
}

// =============================
// Block and Statements
// =============================

BlockStmt* Parser::block() {
    expect(TokenType::LBRACE, "Expected '{'");

    BlockStmt* blk = new BlockStmt();

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        blk->stmts.emplace_back(statement());
    }

    expect(TokenType::RBRACE, "Expected '}'");

    return blk;
}

Stmt* Parser::statement() {
    if (check(TokenType::KW_INT) ||
        check(TokenType::KW_BOOL) ||
        check(TokenType::KW_STRING))
        return varDeclaration();

    if (match(TokenType::KW_PRINT))   return printStmt();
    if (match(TokenType::KW_RETURN))  return returnStmt();
    if (match(TokenType::KW_INPUT))   return inputStmt();
    if (match(TokenType::KW_NEWLINE)) return newlineStmt();
    if (match(TokenType::KW_FOR))     return forStmt();
    if (match(TokenType::KW_WHILE))   return whileStmt();
    if (match(TokenType::KW_IF))      return ifStmt();

    if (check(TokenType::LBRACE)) return block();

    return assignStmt();
}

// =========== FOR LOOP ===========

Stmt* Parser::forStmt() {
    Token varTok = peek();
    expect(TokenType::ID, "Expected loop variable");

    expect(TokenType::ASSIGN, "Expected '=' in for loop");

    Expr* start = expr();

    expect(TokenType::KW_TO, "Expected 'to' in for loop");

    Expr* end = expr();

    BlockStmt* blk = block();

    ForStmt* fs = new ForStmt();
    fs->var = varTok.lexeme;
    fs->start.reset(start);
    fs->end.reset(end);
    fs->block.reset(blk);

    return fs;
}

// =========== WHILE LOOP ===========

Stmt* Parser::whileStmt() {
    expect(TokenType::LPAREN, "Expected '('");
    Expr* cond = expr();
    expect(TokenType::RPAREN, "Expected ')'");
    BlockStmt* blk = block();

    WhileStmt* ws = new WhileStmt();
    ws->cond.reset(cond);
    ws->block.reset(blk);

    return ws;
}

// =========== IF/ELSE ===========

Stmt* Parser::ifStmt() {
    expect(TokenType::LPAREN, "Expected '('");
    Expr* cond = expr();
    expect(TokenType::RPAREN, "Expected ')'");

    BlockStmt* thenBlk = block();

    BlockStmt* elseBlk = nullptr;
    if (match(TokenType::KW_ELSE)) {
        elseBlk = block();
    }

    IfStmt* is = new IfStmt();
    is->cond.reset(cond);
    is->then_block.reset(thenBlk);
    if (elseBlk) is->else_block.reset(elseBlk);

    return is;
}

// =========== ASSIGNMENT ===========

Stmt* Parser::assignStmt() {
    Token nameTok = peek();
    expect(TokenType::ID, "Expected variable name");

    expect(TokenType::ASSIGN, "Expected '='");

    Expr* val = expr();

    expect(TokenType::SEMICOLON, "Expected ';'");

    AssignStmt* as = new AssignStmt();
    as->name = nameTok.lexeme;
    as->value.reset(val);

    return as;
}

// =========== PRINT ===========

Stmt* Parser::printStmt() {
    Expr* value = expr();
    expect(TokenType::SEMICOLON, "Expected ';'");
    PrintStmt* p = new PrintStmt();
    p->expr.reset(value);
    return p;
}

// =========== RETURN ===========

Stmt* Parser::returnStmt() {
    Expr* value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = expr();
    }
    expect(TokenType::SEMICOLON, "Expected ';'");

    ReturnStmt* r = new ReturnStmt();
    if (value) r->value.reset(value);

    return r;
}

// =========== INPUT ===========

Stmt* Parser::inputStmt() {
    Token nameTok = peek();
    expect(TokenType::ID, "Expected variable name after input");
    expect(TokenType::SEMICOLON, "Expected ';'");
    InputStmt* in = new InputStmt();
    in->name = nameTok.lexeme;
    return in;
}

// =========== NEWLINE ===========

Stmt* Parser::newlineStmt() {
    expect(TokenType::SEMICOLON, "Expected ';'");
    return new NewlineStmt();
}

// =============================
// EXPRESSIONS
// =============================

Expr* Parser::expr() { return logic_or(); }

Expr* Parser::logic_or() {
    Expr* left = logic_and();
    while (match(TokenType::OR)) {
        Token op = tokens[current - 1];
        Expr* right = logic_and();
        left = new BinaryExpr(op.lexeme, left, right);
    }
    return left;
}

Expr* Parser::logic_and() {
    Expr* left = equality();
    while (match(TokenType::AND)) {
        Token op = tokens[current - 1];
        Expr* right = equality();
        left = new BinaryExpr(op.lexeme, left, right);
    }
    return left;
}

Expr* Parser::equality() {
    Expr* left = rel();
    while (match(TokenType::EQ) || match(TokenType::NEQ)) {
        Token op = tokens[current - 1];
        Expr* right = rel();
        left = new BinaryExpr(op.lexeme, left, right);
    }
    return left;
}

Expr* Parser::rel() {
    Expr* left = add();
    while (match(TokenType::LT) || match(TokenType::GT) ||
           match(TokenType::LEQ) || match(TokenType::GEQ)) {
        Token op = tokens[current - 1];
        Expr* right = add();
        left = new BinaryExpr(op.lexeme, left, right);
    }
    return left;
}

Expr* Parser::add() {
    Expr* left = mul();
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        Token op = tokens[current - 1];
        Expr* right = mul();
        left = new BinaryExpr(op.lexeme, left, right);
    }
    return left;
}

Expr* Parser::mul() {
    Expr* left = unary();
    while (match(TokenType::MUL) || match(TokenType::DIV) || match(TokenType::MOD)) {
        Token op = tokens[current - 1];
        Expr* right = unary();
        left = new BinaryExpr(op.lexeme, left, right);
    }
    return left;
}

Expr* Parser::unary() {
    if (match(TokenType::NOT) || match(TokenType::MINUS)) {
        Token op = tokens[current - 1];
        Expr* exprNode = unary();
        return new UnaryExpr(op.lexeme, exprNode);
    }
    return primary();
}

Expr* Parser::primary() {
    if (match(TokenType::INT_LITERAL))   return new NumberExpr(tokens[current-1].lexeme);
    if (match(TokenType::BOOL_LITERAL))  return new BoolExpr(tokens[current-1].lexeme);
    if (match(TokenType::STRING_LITERAL))return new StringExpr(tokens[current-1].lexeme);

    if (match(TokenType::LPAREN)) {
        Expr* e = expr();
        expect(TokenType::RPAREN, "Expected ')'");
        return e;
    }

    if (match(TokenType::ID)) {
        Token id = tokens[current - 1];

        // function call?
        if (match(TokenType::LPAREN)) {
            FuncCallExpr* call = new FuncCallExpr(id.lexeme);
            if (!check(TokenType::RPAREN)) {
                do {
                    call->args.emplace_back(expr());
                } while (match(TokenType::COMMA));
            }
            expect(TokenType::RPAREN, "Expected ')'");
            return call;
        }

        return new VarExpr(id.lexeme);
    }

    std::cerr << "Unexpected token at line " << peek().line << "\n";
    exit(1);
}