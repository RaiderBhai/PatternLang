#include "ir.h"
#include <sstream>

std::string TAC::toString() const {
    if (isLabel) {
        if (res.empty()) return "label:";
        return res + ":";
    }
    std::ostringstream out;
    if (op == "assign") {
        out << res << " = " << arg1;
        return out.str();
    }
    if (op == "print") {
        out << "print " << arg1;
        return out.str();
    }
    if (op == "newline") {
        out << "newline";
        return out.str();
    }
    if (op == "call") {
        // res = call name, a1, a2, ...
        out << res << " = call " << arg1;
        if (!arg2.empty()) out << ", " << arg2;
        return out.str();
    }
    if (op == "return") {
        if (arg1.empty()) out << "return";
        else out << "return " << arg1;
        return out.str();
    }
    if (op == "goto") {
        out << "goto " << res;
        return out.str();
    }
    if (op == "ifFalse") {
        out << "ifFalse " << arg1 << " goto " << res;
        return out.str();
    }
    // binary ops: op in {"+","-","*","/","%","==","!=","<",">","<=",">=","&&","||"}
    if (!arg2.empty()) {
        out << res << " = " << arg1 << " " << op << " " << arg2;
        return out.str();
    }
    // fallback unary
    if (!arg1.empty()) {
        out << res << " = " << op << " " << arg1;
        return out.str();
    }
    return "";
}
