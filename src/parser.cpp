#include "parser.h"
#include <iostream>

// ==========================
// Helper functions
// ==========================
Parser::Parser(const std::vector<Token> &toks) : tokens(toks) {}

Token Parser::peek() { return tokens[current]; }

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return tokens[current - 1];
}

bool Parser::isAtEnd() { return peek().type == TokenType::END_OF_FILE; }

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

        if (match(TokenType::KW_FUNC)) {
            prog->decls.emplace_back(funcDeclaration());
            continue;
        }

        if (check(TokenType::KW_INT) || check(TokenType::KW_BOOL) || check(TokenType::KW_STRING)) {
            prog->decls.emplace_back(varDeclaration());
            continue;
        }

        // top-level statement
        prog->decls.emplace_back(statement());
    }

    return prog;
}

// =============================
// Declarations
// =============================
VarDeclStmt* Parser::varDeclaration() {

    if (!(check(TokenType::KW_INT) || check(TokenType::KW_BOOL) || check(TokenType::KW_STRING))) {
        Token p = peek();
        std::cerr << "Parser error at line " << p.line
            << ": Expected type but got '" << p.lexeme << "'\n";
        exit(1);
    }

    Token typeTok = advance();
    Token nameTok = peek();
    expect(TokenType::ID, "Expected identifier");

    auto decl = new VarDeclStmt();
    decl->line = nameTok.line;
    decl->type = typeTok.lexeme;
    decl->name = nameTok.lexeme;

    if (match(TokenType::ASSIGN)) {
        decl->init.reset(expr());
    }

    expect(TokenType::SEMICOLON, "Expected ';'");

    return decl;
}

FuncDecl* Parser::funcDeclaration() {
    Token nameTok = peek();
    expect(TokenType::ID, "Expected function name");

    expect(TokenType::LPAREN, "Expected '('");
    std::vector<FuncParam> params;

    if (!check(TokenType::RPAREN)) {
        do {
            Token typeTok = advance();
            if (!(typeTok.type == TokenType::KW_INT ||
                  typeTok.type == TokenType::KW_BOOL ||
                  typeTok.type == TokenType::KW_STRING)) {
                std::cerr << "Expected parameter type\n";
                exit(1);
            }
            Token pname = peek();
            expect(TokenType::ID, "Expected parameter name");

            params.push_back({ typeTok.lexeme, pname.lexeme });

        } while (match(TokenType::COMMA));
    }

    expect(TokenType::RPAREN, "Expected ')'");

    FuncDecl* f = new FuncDecl();
    f->line = nameTok.line;
    f->name = nameTok.lexeme;
    f->params = params;
    f->body.reset(block());
    return f;
}

// =============================
// Block & Statements
// =============================
BlockStmt* Parser::block() {
    expect(TokenType::LBRACE, "Expected '{'");

    BlockStmt* blk = new BlockStmt();
    blk->line = peek().line;

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        blk->stmts.emplace_back(statement());
    }

    expect(TokenType::RBRACE, "Expected '}'");
    return blk;
}

Stmt* Parser::statement() {

    if (check(TokenType::KW_INT) || check(TokenType::KW_BOOL) || check(TokenType::KW_STRING))
        return varDeclaration();

    if (match(TokenType::KW_PRINT)) return printStmt();
    if (match(TokenType::KW_RETURN)) return returnStmt();
    if (match(TokenType::KW_INPUT)) return inputStmt();
    if (match(TokenType::KW_NEWLINE)) return newlineStmt();
    if (match(TokenType::KW_FOR)) return forStmt();
    if (match(TokenType::KW_WHILE)) return whileStmt();
    if (match(TokenType::KW_IF)) return ifStmt();

    if (check(TokenType::LBRACE)) return block();


    // assignment detection with safe lookahead
    if (check(TokenType::ID)) {
        if (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::ASSIGN) {
            return assignStmt();
        }
        // function call as statement: ID LPAREN ... RPAREN SEMICOLON
        if (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::LPAREN) {
            Token id = advance(); // consume ID
            expect(TokenType::LPAREN, "Expected '('");
            auto call = new FuncCallStmt(id.lexeme, id.line);
            if (!check(TokenType::RPAREN)) {
                do {
                    call->args.emplace_back(expr());
                } while (match(TokenType::COMMA));
            }
            expect(TokenType::RPAREN, "Expected ')'");
            expect(TokenType::SEMICOLON, "Expected ';' after function call statement");
            return call;
        }
    }

    Token p = peek();
    std::cerr << "Parser error at line " << p.line
              << ": Unexpected statement starting with '"
              << p.lexeme << "'\n";
    exit(1);
}

// =============================
// FOR loop
// =============================
Stmt* Parser::forStmt() {
    Token varTok = peek();
    expect(TokenType::ID, "Expected loop variable");

    expect(TokenType::ASSIGN, "Expected '='");

    Expr* start = expr();

    expect(TokenType::KW_TO, "Expected 'to'");

    Expr* end = expr();

    BlockStmt* blk = block();

    auto f = new ForStmt();
    f->line = varTok.line;
    f->var = varTok.lexeme;
    f->start.reset(start);
    f->end.reset(end);
    f->block.reset(blk);

    return f;
}

Stmt* Parser::whileStmt() {
    Token t = peek();
    expect(TokenType::LPAREN, "Expected '('");
    Expr* cond = expr();
    expect(TokenType::RPAREN, "Expected ')'");
    auto w = new WhileStmt();
    w->line = t.line;
    w->cond.reset(cond);
    w->block.reset(block());
    return w;
}

Stmt* Parser::ifStmt() {
    Token t = peek();
    expect(TokenType::LPAREN, "Expected '('");
    Expr* cond = expr();
    expect(TokenType::RPAREN, "Expected ')'");
    auto th = block();
    BlockStmt* el = nullptr;

    if (match(TokenType::KW_ELSE)) el = block();

    auto i = new IfStmt();
    i->line = t.line;
    i->cond.reset(cond);
    i->then_block.reset(th);
    if (el) i->else_block.reset(el);

    return i;
}

Stmt* Parser::assignStmt() {
    Token nameTok = peek();
    expect(TokenType::ID, "Expected variable name");
    expect(TokenType::ASSIGN, "Expected '='");

    Expr* v = expr();
    expect(TokenType::SEMICOLON, "Expected ';'");

    auto a = new AssignStmt();
    a->line = nameTok.line;
    a->name = nameTok.lexeme;
    a->value.reset(v);
    return a;
}

// =============================
// PRINT, RETURN, INPUT
// =============================
Stmt* Parser::printStmt() {
    Token t = peek();
    Expr* e = expr();
    expect(TokenType::SEMICOLON, "Expected ';'");
    auto p = new PrintStmt();
    p->line = t.line;
    p->expr.reset(e);
    return p;
}

Stmt* Parser::returnStmt() {
    Token t = peek();
    Expr* e = nullptr;

    if (!check(TokenType::SEMICOLON)) e = expr();

    expect(TokenType::SEMICOLON, "Expected ';'");
    auto r = new ReturnStmt();
    r->line = t.line;
    if (e) r->value.reset(e);
    return r;
}

Stmt* Parser::inputStmt() {
    Token nameTok = peek();
    expect(TokenType::ID, "Expected variable name");
    expect(TokenType::SEMICOLON, "Expected ';'");
    auto i = new InputStmt();
    i->line = nameTok.line;
    i->name = nameTok.lexeme;
    return i;
}

Stmt* Parser::newlineStmt() {
    Token t = peek();
    expect(TokenType::SEMICOLON, "Expected ';'");
    auto n = new NewlineStmt();
    n->line = t.line;
    return n;
}

// =============================
// EXPRESSIONS
// =============================
Expr* Parser::expr() { return logic_or(); }

Expr* Parser::logic_or() {
    Expr* left = logic_and();
    while (match(TokenType::OR)) {
        Token op = tokens[current - 1];
        left = new BinaryExpr(op.lexeme, left, logic_and(), op.line);
    }
    return left;
}

Expr* Parser::logic_and() {
    Expr* left = equality();
    while (match(TokenType::AND)) {
        Token op = tokens[current - 1];
        left = new BinaryExpr(op.lexeme, left, equality(), op.line);
    }
    return left;
}

Expr* Parser::equality() {
    Expr* left = rel();
    while (match(TokenType::EQ) || match(TokenType::NEQ)) {
        Token op = tokens[current - 1];
        left = new BinaryExpr(op.lexeme, left, rel(), op.line);
    }
    return left;
}

Expr* Parser::rel() {
    Expr* left = add();
    while (match(TokenType::LT) || match(TokenType::GT) ||
           match(TokenType::LEQ) || match(TokenType::GEQ)) {
        Token op = tokens[current - 1];
        left = new BinaryExpr(op.lexeme, left, add(), op.line);
    }
    return left;
}

Expr* Parser::add() {
    Expr* left = mul();
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        Token op = tokens[current - 1];
        left = new BinaryExpr(op.lexeme, left, mul(), op.line);
    }
    return left;
}

Expr* Parser::mul() {
    Expr* left = unary();
    while (match(TokenType::MUL) || match(TokenType::DIV) || match(TokenType::MOD)) {
        Token op = tokens[current - 1];
        left = new BinaryExpr(op.lexeme, left, unary(), op.line);
    }
    return left;
}

Expr* Parser::unary() {
    if (match(TokenType::NOT) || match(TokenType::MINUS)) {
        Token op = tokens[current - 1];
        return new UnaryExpr(op.lexeme, unary(), op.line);
    }
    return primary();
}

Expr* Parser::primary() {

    if (match(TokenType::INT_LITERAL))
        return new NumberExpr(tokens[current - 1].lexeme, tokens[current - 1].line);

    if (match(TokenType::BOOL_LITERAL))
        return new BoolExpr(tokens[current - 1].lexeme, tokens[current - 1].line);

    if (match(TokenType::STRING_LITERAL))
        return new StringExpr(tokens[current - 1].lexeme, tokens[current - 1].line);

    if (match(TokenType::LPAREN)) {
        Expr* e = expr();
        expect(TokenType::RPAREN, "Expected ')'");
        return e;
    }

    if (match(TokenType::ID)) {
        Token id = tokens[current - 1];

        if (match(TokenType::LPAREN)) {
            auto call = new FuncCallExpr(id.lexeme, id.line);

            if (!check(TokenType::RPAREN)) {
                do {
                    call->args.emplace_back(expr());
                } while (match(TokenType::COMMA));
            }

            expect(TokenType::RPAREN, "Expected ')'");
            return call;
        }

        return new VarExpr(id.lexeme, id.line);
    }

    std::cerr << "Unexpected token at line " << peek().line << "\n";
    exit(1);
}

