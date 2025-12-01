#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>

// =====================
// Base AST Node
// =====================
struct ASTNode {
    virtual ~ASTNode() = default;
};

// =====================
// Expressions
// =====================

struct Expr : ASTNode {};

struct NumberExpr : Expr {
    std::string value;
    NumberExpr(const std::string &v) : value(v) {}
};

struct BoolExpr : Expr {
    std::string value;
    BoolExpr(const std::string &v) : value(v) {}
};

struct StringExpr : Expr {
    std::string value;
    StringExpr(const std::string &v) : value(v) {}
};

struct VarExpr : Expr {
    std::string name;
    VarExpr(const std::string &n) : name(n) {}
};

struct BinaryExpr : Expr {
    std::string op;
    std::unique_ptr<Expr> left, right;
    BinaryExpr(std::string o, Expr* l, Expr* r)
        : op(o), left(l), right(r) {}
};

struct UnaryExpr : Expr {
    std::string op;
    std::unique_ptr<Expr> expr;
    UnaryExpr(std::string o, Expr* e) : op(o), expr(e) {}
};

struct FuncCallExpr : Expr {
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;
    FuncCallExpr(const std::string &n) : name(n) {}
};

// =====================
// Statements
// =====================

struct Stmt : ASTNode {};

struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> stmts;
};

struct VarDeclStmt : Stmt {
    std::string type;
    std::string name;
    std::unique_ptr<Expr> init;
};

struct AssignStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> value;
};

struct PrintStmt : Stmt {
    std::unique_ptr<Expr> expr;
};

struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;
};

struct InputStmt : Stmt {
    std::string name;
};

struct NewlineStmt : Stmt {};

struct IfStmt : Stmt {
    std::unique_ptr<Expr> cond;
    std::unique_ptr<BlockStmt> then_block;
    std::unique_ptr<BlockStmt> else_block;
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> cond;
    std::unique_ptr<BlockStmt> block;
};

struct ForStmt : Stmt {
    std::string var;
    std::unique_ptr<Expr> start;
    std::unique_ptr<Expr> end;
    std::unique_ptr<BlockStmt> block;
};

// =====================
// Function Declaration
// =====================

struct FuncParam {
    std::string type;
    std::string name;
};

struct FuncDecl : ASTNode {
    std::string name;
    std::vector<FuncParam> params;
    std::unique_ptr<BlockStmt> body;
};

// =====================
// Program Root
// =====================

struct Program : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> decls;
};

#endif
