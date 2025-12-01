#include "interpreter.h"
#include <iostream>
#include <stdexcept>

using namespace std;

// ---------- Value helpers ----------
string Value::toString() const {
    if (std::holds_alternative<int>(v)) return std::to_string(std::get<int>(v));
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? "true" : "false";
    return std::get<std::string>(v);
}
bool Value::isInt() const { return std::holds_alternative<int>(v); }
bool Value::isBool() const { return std::holds_alternative<bool>(v); }
bool Value::isString() const { return std::holds_alternative<std::string>(v); }
int Value::asInt() const { return std::get<int>(v); }
bool Value::asBool() const { return std::get<bool>(v); }
const string& Value::asString() const { return std::get<std::string>(v); }

// ---------- Interpreter ----------
Interpreter::Interpreter() {
    // start with global scope
    env.emplace_back();
}

void Interpreter::pushScope() { env.emplace_back(); }
void Interpreter::popScope() {
    if (!env.empty()) env.pop_back();
}

void Interpreter::setVar(const string &name, const Value &val) {
    // set in nearest scope where exists, else in current scope
    for (int i = (int)env.size()-1; i >= 0; --i) {
        auto &m = env[i];
        auto it = m.find(name);
        if (it != m.end()) { it->second = val; return; }
    }
    // not found -> set in current scope
    env.back()[name] = val;
}

bool Interpreter::existsVarInCurrent(const string &name) {
    if (env.empty()) return false;
    return env.back().find(name) != env.back().end();
}

bool Interpreter::existsVar(const string &name) {
    for (int i = (int)env.size()-1; i >= 0; --i)
        if (env[i].find(name) != env[i].end()) return true;
    return false;
}

Value Interpreter::getVar(const string &name) {
    for (int i = (int)env.size()-1; i >= 0; --i) {
        auto &m = env[i];
        auto it = m.find(name);
        if (it != m.end()) return it->second;
    }
    throw runtime_error("Runtime error: use of undeclared variable '" + name + "'");
}

void Interpreter::loadProgram(Program* p) {
    funcs.clear();
    // register functions
    for (auto &node : p->decls) {
        if (auto f = dynamic_cast<FuncDecl*>(node.get())) {
            funcs[f->name] = f;
        } else if (auto v = dynamic_cast<VarDeclStmt*>(node.get())) {
            // create global variables if initializer present or default
            if (v->init) {
                Value val = evalExpr(v->init.get());
                env.front()[v->name] = val;
            } else {
                // default values: int = 0, bool=false, string=""
                if (v->type == "int") env.front()[v->name] = Value(0);
                else if (v->type == "bool") env.front()[v->name] = Value(false);
                else env.front()[v->name] = Value(std::string(""));
            }
        }
    }
}

void Interpreter::run(Program* p) {
    // load functions + initialize globals
    loadProgram(p);

    // execute top-level statements (non function / non var)
    for (auto &node : p->decls) {
        if (auto s = dynamic_cast<Stmt*>(node.get())) {
            execStmt(s);
        }
    }
}

// ---------- execution ----------
void Interpreter::execStmt(Stmt* s) {
    if (!s) return;

    // VarDeclStmt (local)
    if (auto v = dynamic_cast<VarDeclStmt*>(s)) {
        if (v->init) {
            Value val = evalExpr(v->init.get());
            env.back()[v->name] = val;
        } else {
            if (v->type == "int") env.back()[v->name] = Value(0);
            else if (v->type == "bool") env.back()[v->name] = Value(false);
            else env.back()[v->name] = Value(std::string(""));
        }
        return;
    }

    if (auto a = dynamic_cast<AssignStmt*>(s)) {
        Value rv = evalExpr(a->value.get());
        setVar(a->name, rv);
        return;
    }

    if (auto p = dynamic_cast<PrintStmt*>(s)) {
        Value val = evalExpr(p->expr.get());
        builtin_print(val);
        return;
    }

    if (auto in = dynamic_cast<InputStmt*>(s)) {
        builtin_input(in->name);
        return;
    }

    if (dynamic_cast<NewlineStmt*>(s)) {
        builtin_newline();
        return;
    }

    if (auto r = dynamic_cast<ReturnStmt*>(s)) {
        if (r->value) {
            Value rv = evalExpr(r->value.get());
            throw ReturnException(rv);
        } else {
            throw ReturnException();
        }
    }

    if (auto f = dynamic_cast<ForStmt*>(s)) { execFor(f); return; }
    if (auto w = dynamic_cast<WhileStmt*>(s)) { execWhile(w); return; }
    if (auto i = dynamic_cast<IfStmt*>(s)) { execIf(i); return; }
    if (auto b = dynamic_cast<BlockStmt*>(s)) { execBlock(b); return; }

    // unknown stmt -> ignore
}

Value Interpreter::evalExpr(Expr* e) {
    if (!e) return Value();

    if (auto n = dynamic_cast<NumberExpr*>(e)) {
        return Value(std::stoi(n->value));
    }
    if (auto b = dynamic_cast<BoolExpr*>(e)) {
        if (b->value == "true") return Value(true);
        return Value(false);
    }
    if (auto s = dynamic_cast<StringExpr*>(e)) {
        return Value(s->value);
    }
    if (auto v = dynamic_cast<VarExpr*>(e)) {
        return getVar(v->name);
    }
    if (auto call = dynamic_cast<FuncCallExpr*>(e)) {
        vector<Value> args;
        for (auto &a : call->args) args.push_back(evalExpr(a.get()));
        return callFunction(call->name, args);
    }
    if (auto u = dynamic_cast<UnaryExpr*>(e)) {
        Value sub = evalExpr(u->expr.get());
        if (u->op == "!") {
            return Value(!sub.asBool());
        }
        if (u->op == "-") {
            return Value(-sub.asInt());
        }
    }
    if (auto bin = dynamic_cast<BinaryExpr*>(e)) {
        Value L = evalExpr(bin->left.get());
        Value R = evalExpr(bin->right.get());
        const string &op = bin->op;
        if (op == "+") {
            if (L.isInt() && R.isInt()) return Value(L.asInt() + R.asInt());
            // string concat if any is string
            if (L.isString() || R.isString())
                return Value(L.toString() + R.toString());
        }
        if (op == "-") return Value(L.asInt() - R.asInt());
        if (op == "*") return Value(L.asInt() * R.asInt());
        if (op == "/") return Value(L.asInt() / R.asInt());
        if (op == "%") return Value(L.asInt() % R.asInt());

        if (op == "==") return Value(L.toString() == R.toString());
        if (op == "!=") return Value(L.toString() != R.toString());
        if (op == "<") return Value(L.asInt() < R.asInt());
        if (op == ">") return Value(L.asInt() > R.asInt());
        if (op == "<=") return Value(L.asInt() <= R.asInt());
        if (op == ">=") return Value(L.asInt() >= R.asInt());
        if (op == "&&") return Value(L.asBool() && R.asBool());
        if (op == "||") return Value(L.asBool() || R.asBool());
    }

    // fallback
    return Value();
}

void Interpreter::execBlock(BlockStmt* b) {
    pushScope();
    for (auto &s : b->stmts) {
        execStmt(s.get());
    }
    popScope();
}

void Interpreter::execFor(ForStmt* f) {
    // create loop variable in current scope (or use existing)
    if (!existsVarInCurrent(f->var)) env.back()[f->var] = Value(0);
    Value start = evalExpr(f->start.get());
    Value end = evalExpr(f->end.get());
    for (int i = start.asInt(); i <= end.asInt(); ++i) {
        setVar(f->var, Value(i));
        // run block
        try {
            execBlock(f->block.get());
        } catch (ReturnException &re) {
            // propagate return up
            throw;
        }
    }
}

void Interpreter::execWhile(WhileStmt* w) {
    while (evalExpr(w->cond.get()).asBool()) {
        try {
            execBlock(w->block.get());
        } catch (ReturnException &re) {
            throw;
        }
    }
}

void Interpreter::execIf(IfStmt* i) {
    if (evalExpr(i->cond.get()).asBool()) {
        execBlock(i->then_block.get());
    } else if (i->else_block) {
        execBlock(i->else_block.get());
    }
}

// ---------- functions ----------
Value Interpreter::callFunction(const string &name, const vector<Value> &args) {
    // built-in: print, input, newline handled elsewhere as statements.
    auto it = funcs.find(name);
    if (it == funcs.end()) {
        throw runtime_error("Call to undeclared function '" + name + "'");
    }
    FuncDecl* f = it->second;
    // create new scope for function and set params
    pushScope();
    for (size_t i = 0; i < f->params.size(); ++i) {
        const auto &p = f->params[i];
        if (i < args.size()) env.back()[p.name] = args[i];
        else {
            // missing arg -> default 0/false/""
            if (p.type == "int") env.back()[p.name] = Value(0);
            else if (p.type == "bool") env.back()[p.name] = Value(false);
            else env.back()[p.name] = Value(string(""));
        }
    }
    // execute function body; capture return via exception
    try {
        if (f->body) {
            for (auto &s : f->body->stmts) execStmt(s.get());
        }
    } catch (ReturnException &re) {
        popScope();
        if (re.hasValue) return re.value;
        return Value(); // void -> default 0
    }
    popScope();
    return Value(); // no return -> default 0
}

// ---------- builtins ----------
void Interpreter::builtin_print(const Value &v) {
    // print without newline (matching grammar: print expr ;)
    cout << v.toString();
}

void Interpreter::builtin_newline() {
    cout << "\n";
}

void Interpreter::builtin_input(const string &name) {
    string line;
    if (!std::getline(cin, line)) line = "";
    // store into var if exists, else create string var
    if (existsVar(name)) {
        // attempt to coerce based on current var type
        Value cur = getVar(name);
        if (cur.isInt()) {
            try { int x = stoi(line); setVar(name, Value(x)); return; } catch(...) { setVar(name, Value(0)); return; }
        }
        if (cur.isBool()) {
            if (line == "true") setVar(name, Value(true));
            else setVar(name, Value(false));
            return;
        }
        setVar(name, Value(line));
    } else {
        // create as string
        setVar(name, Value(line));
    }
}
