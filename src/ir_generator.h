#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include "ir.h"
#include "ast.h"
#include <string>
#include <vector>
#include <memory>

class IRGenerator {
public:
    IRGenerator();

    // generate TAC for whole program (functions + globals + statements)
    void generate(Program* program);

    // get produced TAC
    const TACList& getTAC() const { return code; }

private:
    TACList code;
    int tmpCount;
    int labelCount;

    std::string newTemp();
    std::string newLabel(const std::string &base = "L");

    // generation helpers
    // returns an operand name (temp or variable or literal)
    std::string genExpr(Expr* e);

    void genStmt(Stmt* s);
    void genBlock(BlockStmt* b);
    void genFunc(FuncDecl* f);

    // helper to append an instruction
    void emit(const TAC &t) { code.push_back(t); }
    void emitAssign(const std::string &dst, const std::string &src);
};

#endif
