#include "sema.h"
#include <cstdlib>
#include <functional>

// Helper: convert token/type name to internal Type
Type SemanticAnalyzer::stringToType(const std::string &s) {
    if (s == "int") return Type(TypeKind::TYPE_INT);
    if (s == "bool") return Type(TypeKind::TYPE_BOOL);
    if (s == "string") return Type(TypeKind::TYPE_STRING);
    // default unknown
    return Type(TypeKind::TYPE_UNKNOWN);
}

void SemanticAnalyzer::error(const std::string &msg, int line) {
    std::cerr << "Semantic error (line " << line << "): " << msg << "\n";
    exit(1);
}

// Forward declarations for recursive traversal
Type analyzeExpr(SemanticAnalyzer &sa, Expr* e);
void analyzeStmt(SemanticAnalyzer &sa, Stmt* s);
void analyzeFuncDecl(SemanticAnalyzer &sa, FuncDecl* f);

void SemanticAnalyzer::analyze(Program* program) {
    // Phase 0: Collect function declarations (signatures) and global vars
    // First pass: register functions and global variables
    for (auto &nodePtr : program->decls) {
        // Function declarations
        if (auto f = dynamic_cast<FuncDecl*>(nodePtr.get())) {
            Symbol sym;
            sym.name = f->name;
            sym.isFunction = true;
            // params
            for (auto &p : f->params) {
                sym.paramTypes.push_back(stringToType(p.type));
            }
            // For now set return type to unknown; we will determine by checking return statements.
            // But parser currently doesn't store return type in FuncDecl. We will assume functions return int by usage.
            // If you want typed functions, extend grammar. For now assume functions return int (or detect from returns).
            sym.returnType = Type(TypeKind::TYPE_UNKNOWN);
            if (!symtab.insertGlobal(sym)) {
                error("Redefinition of function '" + sym.name + "'", 0);
            }
        } else if (auto v = dynamic_cast<VarDeclStmt*>(nodePtr.get())) {
            Symbol s;
            s.name = v->name;
            s.type = stringToType(v->type);
            if (s.type.kind == TypeKind::TYPE_UNKNOWN) {
                error("Unknown type for variable '" + s.name + "'", 0);
            }
            if (!symtab.insert(s)) {
                error("Redefinition of variable '" + s.name + "'", 0);
            }
        } else {
            // top-level stmt: ignore in first pass
        }
    }

    // Second pass: analyze nodes (functions bodies and top-level statements)
    for (auto &nodePtr : program->decls) {
        if (auto f = dynamic_cast<FuncDecl*>(nodePtr.get())) {
            analyzeFuncDecl(*this, f);
        } else if (auto v = dynamic_cast<VarDeclStmt*>(nodePtr.get())) {
            if (v->init) {
                // check initializer type matches declaration
                Type rt = analyzeExpr(*this, v->init.get());
                Type dt = stringToType(v->type);
                if (dt.kind == TypeKind::TYPE_UNKNOWN) {
                    error("Unknown type for variable '" + v->name + "'", 0);
                }
                if (rt != dt) {
                    error("Type mismatch in initialization of '" + v->name + "': expected " + dt.toString() + " but got " + rt.toString(), v->init->line);
                }
            }
        } else if (auto s = dynamic_cast<Stmt*>(nodePtr.get())) {
            analyzeStmt(*this, s);
        } else {
            // ignore
        }
    }
}

// ----------------- helpers -----------------

Type analyzeExpr(SemanticAnalyzer &sa, Expr* e) {
    if (!e) return Type(TypeKind::TYPE_UNKNOWN);

    if (auto n = dynamic_cast<NumberExpr*>(e)) {
        return Type(TypeKind::TYPE_INT);
    }
    if (auto b = dynamic_cast<BoolExpr*>(e)) {
        return Type(TypeKind::TYPE_BOOL);
    }
    if (auto s = dynamic_cast<StringExpr*>(e)) {
        return Type(TypeKind::TYPE_STRING);
    }
    if (auto v = dynamic_cast<VarExpr*>(e)) {
        Symbol* sym = sa.symtab.lookup(v->name);
        if (!sym) sa.error("Use of undeclared variable '" + v->name + "'", 0);
        if (sym->isFunction) sa.error("'" + v->name + "' is a function, not a variable", 0);
        return sym->type;
    }
    if (auto call = dynamic_cast<FuncCallExpr*>(e)) {
        Symbol* sym = sa.symtab.lookup(call->name);
        if (!sym || !sym->isFunction) sa.error("Call to undeclared function '" + call->name + "'", 0);

        if (sym->paramTypes.size() != call->args.size()) {
            sa.error("Function '" + call->name + "' expects " + std::to_string(sym->paramTypes.size()) +
                     " arguments but got " + std::to_string(call->args.size()), 0);
        }
        for (size_t i = 0; i < call->args.size() && i < sym->paramTypes.size(); ++i) {
            Type at = analyzeExpr(sa, call->args[i].get());
            if (at != sym->paramTypes[i]) {
                sa.error("Type mismatch in argument " + std::to_string(i+1) + " of function '" + call->name +
                         "': expected " + sym->paramTypes[i].toString() + " but got " + at.toString(), 0);
            }
        }
        // return type is whatever sym.returnType is, or unknown
        return sym->returnType.kind == TypeKind::TYPE_UNKNOWN ? Type(TypeKind::TYPE_INT) : sym->returnType;
    }
    if (auto u = dynamic_cast<UnaryExpr*>(e)) {
        Type sub = analyzeExpr(sa, u->expr.get());
        if (u->op == "!") {
            if (sub.kind != TypeKind::TYPE_BOOL) sa.error("Operator '!' requires bool operand", 0);
            return Type(TypeKind::TYPE_BOOL);
        }
        if (u->op == "-") {
            if (sub.kind != TypeKind::TYPE_INT) sa.error("Unary '-' requires int operand", 0);
            return Type(TypeKind::TYPE_INT);
        }
    }
    if (auto b = dynamic_cast<BinaryExpr*>(e)) {
        Type L = analyzeExpr(sa, b->left.get());
        Type R = analyzeExpr(sa, b->right.get());
        const std::string &op = b->op;

        if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
            if (L.kind != TypeKind::TYPE_INT || R.kind != TypeKind::TYPE_INT)
                sa.error("Arithmetic operator '" + op + "' requires integer operands", 0);
            return Type(TypeKind::TYPE_INT);
        }
        if (op == "==" || op == "!=") {
            if (L.kind != R.kind) sa.error("Equality operator requires operands of same type", 0);
            return Type(TypeKind::TYPE_BOOL);
        }
        if (op == "<" || op == ">" || op == "<=" || op == ">=") {
            if (L.kind != TypeKind::TYPE_INT || R.kind != TypeKind::TYPE_INT)
                sa.error("Relational operator '" + op + "' requires integer operands", 0);
            return Type(TypeKind::TYPE_BOOL);
        }
        if (op == "&&" || op == "||") {
            if (L.kind != TypeKind::TYPE_BOOL || R.kind != TypeKind::TYPE_BOOL)
                sa.error("Logical operator '" + op + "' requires boolean operands", 0);
            return Type(TypeKind::TYPE_BOOL);
        }
        // fallback
        return Type(TypeKind::TYPE_UNKNOWN);
    }

    // unknown expr
    return Type(TypeKind::TYPE_UNKNOWN);
}

void analyzeStmt(SemanticAnalyzer &sa, Stmt* s) {
    if (!s) return;
    // VarDeclStmt (local)
    if (auto v = dynamic_cast<VarDeclStmt*>(s)) {
        Type dt = sa.stringToType(v->type);
        if (dt.kind == TypeKind::TYPE_UNKNOWN) sa.error("Unknown type in declaration of '" + v->name + "'", 0);
        if (sa.symtab.existsInCurrent(v->name)) sa.error("Redeclaration of variable '" + v->name + "'", 0);
        Symbol sym;
        sym.name = v->name;
        sym.type = dt;
        sa.symtab.insert(sym);

        if (v->init) {
            Type rt = analyzeExpr(sa, v->init.get());
            if (rt != dt) sa.error("Type mismatch in initialization of '" + v->name + "': expected " + dt.toString() + " but got " + rt.toString(), 0);
        }
        return;
    }
    // Assignment
    if (auto a = dynamic_cast<AssignStmt*>(s)) {
        Symbol* sym = sa.symtab.lookup(a->name);
        if (!sym) sa.error("Assignment to undeclared variable '" + a->name + "'", 0);
        if (sym->isFunction) sa.error("Cannot assign to function '" + a->name + "'", 0);
        Type rt = analyzeExpr(sa, a->value.get());
        if (rt != sym->type) sa.error("Type mismatch in assignment to '" + a->name + "': expected " + sym->type.toString() + " but got " + rt.toString(), 0);
        return;
    }
    // Print
    if (auto p = dynamic_cast<PrintStmt*>(s)) {
        analyzeExpr(sa, p->expr.get());
        return;
    }
    // Input
    if (auto in = dynamic_cast<InputStmt*>(s)) {
        Symbol* sym = sa.symtab.lookup(in->name);
        if (!sym) sa.error("Input to undeclared variable '" + in->name + "'", 0);
        return;
    }
    // Newline: nothing
    if (dynamic_cast<NewlineStmt*>(s)) return;

    // Return
    if (auto r = dynamic_cast<ReturnStmt*>(s)) {
        if (!sa.inFunction) sa.error("Return statement outside of function", 0);
        if (r->value) {
            Type rt = analyzeExpr(sa, r->value.get());
            if (sa.currentFunctionReturnType.kind != TypeKind::TYPE_UNKNOWN && rt != sa.currentFunctionReturnType)
                sa.error("Return type mismatch: expected " + sa.currentFunctionReturnType.toString() + " but got " + rt.toString(), 0);
        }
        return;
    }

    // ForStmt
    if (auto f = dynamic_cast<ForStmt*>(s)) {
        // loop var must be declared before or treated as new?
        // we'll allow loop variable as new local int
        if (sa.symtab.existsInCurrent(f->var)) {
            Symbol* vs = sa.symtab.lookup(f->var);
            if (vs->type.kind != TypeKind::TYPE_INT) sa.error("Loop variable '" + f->var + "' must be int", 0);
        } else {
            Symbol vsym; vsym.name = f->var; vsym.type = Type(TypeKind::TYPE_INT); sa.symtab.insert(vsym);
        }
        Type st = analyzeExpr(sa, f->start.get());
        Type en = analyzeExpr(sa, f->end.get());
        if (st.kind != TypeKind::TYPE_INT || en.kind != TypeKind::TYPE_INT) sa.error("For loop bounds must be integers", 0);

        sa.symtab.pushScope();
        analyzeStmt(sa, f->block.get()); // block is a BlockStmt handled below
        sa.symtab.popScope();
        return;
    }

    // WhileStmt
    if (auto w = dynamic_cast<WhileStmt*>(s)) {
        Type ct = analyzeExpr(sa, w->cond.get());
        if (ct.kind != TypeKind::TYPE_BOOL) sa.error("While condition must be boolean", 0);
        sa.symtab.pushScope();
        analyzeStmt(sa, w->block.get());
        sa.symtab.popScope();
        return;
    }

    // IfStmt
    if (auto i = dynamic_cast<IfStmt*>(s)) {
        Type ct = analyzeExpr(sa, i->cond.get());
        if (ct.kind != TypeKind::TYPE_BOOL) sa.error("If condition must be boolean", 0);
        sa.symtab.pushScope();
        analyzeStmt(sa, i->then_block.get());
        sa.symtab.popScope();
        if (i->else_block) {
            sa.symtab.pushScope();
            analyzeStmt(sa, i->else_block.get());
            sa.symtab.popScope();
        }
        return;
    }

    // BlockStmt
    if (auto b = dynamic_cast<BlockStmt*>(s)) {
        sa.symtab.pushScope();
        for (auto &st : b->stmts) analyzeStmt(sa, st.get());
        sa.symtab.popScope();
        return;
    }

    // Unknown statement
}

// Analyze function body
void analyzeFuncDecl(SemanticAnalyzer &sa, FuncDecl* f) {
    // set current function return type: parser does not store declared return type.
    // We'll try to infer: if function has return statements, take first return type; else void
    sa.inFunction = true;
    sa.currentFunctionReturnType = Type(TypeKind::TYPE_UNKNOWN);

    // create function scope and insert params
    sa.symtab.pushScope();
    for (auto &p : f->params) {
        Type pt = sa.stringToType(p.type);
        Symbol s; s.name = p.name; s.type = pt;
        if (!sa.symtab.insert(s)) sa.error("Parameter name '" + p.name + "' duplicated", 0);
    }

    // scan function body for returns and statements
    // We'll walk body statements; when we encounter a return with a value we set currentFunctionReturnType if unknown, else check match
    std::function<void(Stmt*)> walkStmt;
    walkStmt = [&](Stmt* s) {
        if (!s) return;
        if (auto r = dynamic_cast<ReturnStmt*>(s)) {
            if (r->value) {
                Type rt = analyzeExpr(sa, r->value.get());
                if (sa.currentFunctionReturnType.kind == TypeKind::TYPE_UNKNOWN) {
                    sa.currentFunctionReturnType = rt;
                } else {
                    if (sa.currentFunctionReturnType != rt) sa.error("Inconsistent return types in function '" + f->name + "'", 0);
                }
            } else {
                // return without value -> void
                if (sa.currentFunctionReturnType.kind == TypeKind::TYPE_UNKNOWN) {
                    sa.currentFunctionReturnType = Type(TypeKind::TYPE_VOID);
                } else if (sa.currentFunctionReturnType.kind != TypeKind::TYPE_VOID) {
                    sa.error("Inconsistent return types (void vs non-void) in function '" + f->name + "'", 0);
                }
            }
            return;
        }

        // for nested statements, reuse analyzeStmt but be careful about recursion / scoping
        if (auto b = dynamic_cast<BlockStmt*>(s)) {
            sa.symtab.pushScope();
            for (auto &st : b->stmts) walkStmt(st.get());
            sa.symtab.popScope();
            return;
        }
        if (auto ifs = dynamic_cast<IfStmt*>(s)) {
            analyzeExpr(sa, ifs->cond.get());
            sa.symtab.pushScope();
            walkStmt(ifs->then_block.get());
            sa.symtab.popScope();
            if (ifs->else_block) {
                sa.symtab.pushScope();
                walkStmt(ifs->else_block.get());
                sa.symtab.popScope();
            }
            return;
        }
        if (auto fs = dynamic_cast<ForStmt*>(s)) {
            analyzeExpr(sa, fs->start.get());
            analyzeExpr(sa, fs->end.get());
            sa.symtab.pushScope();
            walkStmt(fs->block.get());
            sa.symtab.popScope();
            return;
        }
        if (auto ws = dynamic_cast<WhileStmt*>(s)) {
            analyzeExpr(sa, ws->cond.get());
            sa.symtab.pushScope();
            walkStmt(ws->block.get());
            sa.symtab.popScope();
            return;
        }

        // general statements: assignments, prints, etc. Use the analyzer's analyzeStmt to handle them
        analyzeStmt(sa, s);
    };

    // Walk the body
    if (f->body) {
        for (auto &s : f->body->stmts) walkStmt(s.get());
    }

    // save inferred return type into the function symbol
    Symbol* fsym = sa.symtab.lookup(f->name); // Should find function symbol in outer scope
    if (fsym && fsym->isFunction) {
        if (sa.currentFunctionReturnType.kind != TypeKind::TYPE_UNKNOWN) {
            fsym->returnType = sa.currentFunctionReturnType;
        } else {
            fsym->returnType = Type(TypeKind::TYPE_VOID);
        }
    }

    sa.symtab.popScope();
    sa.inFunction = false;
    sa.currentFunctionReturnType = Type(TypeKind::TYPE_VOID);
}

