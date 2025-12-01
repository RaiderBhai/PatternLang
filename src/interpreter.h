#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include <string>
#include <variant>
#include <unordered_map>
#include <vector>
#include <memory>

struct Value {
    // store int, bool, string
    std::variant<int, bool, std::string> v;
    Value() : v(0) {}
    Value(int x) : v(x) {}
    Value(bool b) : v(b) {}
    Value(const std::string &s) : v(s) {}
    std::string toString() const;
    bool isInt() const;
    bool isBool() const;
    bool isString() const;
    int asInt() const;
    bool asBool() const;
    const std::string& asString() const;
};

struct ReturnException {
    bool hasValue;
    Value value;
    ReturnException() : hasValue(false), value() {}
    ReturnException(const Value &v) : hasValue(true), value(v) {}
};

class Interpreter {
    // variable scopes: vector of maps, last = top scope
    std::vector<std::unordered_map<std::string, Value>> env;
    // function table: name -> FuncDecl*
    std::unordered_map<std::string, FuncDecl*> funcs;

public:
    Interpreter();
    // build function table (and optionally populate globals)
    void loadProgram(Program* p);
    // execute program (top-level statements & globals)
    void run(Program* p);

private:
    // env helpers
    void pushScope();
    void popScope();
    void setVar(const std::string &name, const Value &val);
    bool existsVarInCurrent(const std::string &name);
    bool existsVar(const std::string &name);
    Value getVar(const std::string &name);

    // execution
    void execStmt(Stmt* s);
    Value evalExpr(Expr* e);

    // statement helpers
    void execBlock(BlockStmt* b);
    void execFor(ForStmt* f);
    void execWhile(WhileStmt* w);
    void execIf(IfStmt* i);

    // function call
    Value callFunction(const std::string &name, const std::vector<Value> &args);

    // built-ins
    void builtin_print(const Value &v);
    void builtin_newline();
    void builtin_input(const std::string &name); // stores into variable
};

#endif
