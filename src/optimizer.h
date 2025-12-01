#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "ir.h"
#include <vector>
#include <string>

class Optimizer {
public:
    // take input TAC (const) and return a new optimized TACList
    TACList optimize(const TACList &code);

private:
    // these work on a mutable TACList (the copied 'out' inside optimize)
    bool constantFold(TACList &code);
    bool strengthReduce(TACList &code);
    bool copyPropagate(TACList &code);
    bool deadCodeElim(TACList &code);

    // helpers
    static bool isTemp(const std::string &s);
    static bool isIntLiteral(const std::string &s);
    static bool isBoolLiteral(const std::string &s);
    static bool isQuotedString(const std::string &s);
    static long long toInt(const std::string &s);
};

#endif
