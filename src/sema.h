#ifndef SEMA_H
#define SEMA_H

#include "ast.h"
#include "symbol_table.h"
#include <vector>

class SemanticAnalyzer {
    SymbolTable symtab;
    // current function return type (for checking return statements)
    Type currentFunctionReturnType = Type(TypeKind::TYPE_VOID);
    bool inFunction = false;

    // helper
    Type stringToType(const std::string &s);
    void error(const std::string &msg, int line);

public:
    SemanticAnalyzer() = default;
    // entry point
    void analyze(Program* program);
    
    friend Type analyzeExpr(SemanticAnalyzer&, Expr*);
    friend void analyzeStmt(SemanticAnalyzer&, Stmt*);
    friend void analyzeFuncDecl(SemanticAnalyzer&, FuncDecl*);

};

#endif

