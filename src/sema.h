#ifndef SEMA_H
#define SEMA_H

#include "ast.h"
#include "symbol_table.h"
#include <vector>

class SemanticAnalyzer {
    SymbolTable symtab;

    Type currentFunctionReturnType = Type(TypeKind::TYPE_VOID);
    bool inFunction = false;

    Type stringToType(const std::string &s);
    void error(const std::string &msg, int line);

public:
    SemanticAnalyzer() = default;

    void analyze(Program* program);

    // NEW: allow main.cpp to print final symbol table
    SymbolTable& getSymbolTable() { return symtab; }

    friend Type analyzeExpr(SemanticAnalyzer&, Expr*);
    friend void analyzeStmt(SemanticAnalyzer&, Stmt*);
    friend void analyzeFuncDecl(SemanticAnalyzer&, FuncDecl*);
};

#endif
