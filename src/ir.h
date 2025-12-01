#ifndef IR_H
#define IR_H

#include <string>
#include <vector>

struct TAC {
    // op like "+", "*", "assign", "print", "call", "return", "label", "goto", "ifFalse"
    std::string op;
    std::string arg1;
    std::string arg2;
    std::string res;   // result or target
    bool isLabel = false;

    std::string toString() const;
};

using TACList = std::vector<TAC>;

#endif
