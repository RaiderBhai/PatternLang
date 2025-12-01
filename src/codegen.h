#ifndef CODEGEN_H
#define CODEGEN_H
#include "ir.h"
#include "ast.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class CodeGenerator {
public:
    // generate C++ source from optimized TAC and program AST
    // returns the C++ source as a string
    std::string generate(const TACList &tac, Program* program);
private:
    // helpers
    void collectVarTypes(Program* program);
    std::string makeExprOperand(const std::string &op) const;
    bool isLiteral(const std::string &s) const;
    bool isTemp(const std::string &s) const;
    bool isQuotedString(const std::string &s) const;
    bool isBoolLiteral(const std::string &s) const;
    bool isIntLiteral(const std::string &s) const;
    // choose a C++ type string for a temp based on scanned usages
    std::string decideTempType(const std::string &temp,
                               const TACList &tac,
                               size_t startIdx, size_t endIdx) const;

    // mapping of variable name -> type ("int","bool","string")
    std::unordered_map<std::string, std::string> varTypes;
    // set of declared locals in current context to avoid redeclaring
    std::unordered_map<std::string, bool> declared;
};
#endif
