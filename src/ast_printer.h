#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "ast.h"
#include <iostream>
#include <string>

class ASTPrinter {
    int indent = 0;

    void tab() {
        for (int i = 0; i < indent; i++) std::cout << "  ";
    }

    // ==========================
    // Expressions
    // ==========================
    void printExpr(Expr* e) {
        if (!e) { tab(); std::cout << "(null expr)\n"; return; }

        if (auto n = dynamic_cast<NumberExpr*>(e)) {
            tab(); std::cout << "Number(" << n->value << ")\n"; return;
        }
        if (auto b = dynamic_cast<BoolExpr*>(e)) {
            tab(); std::cout << "Bool(" << b->value << ")\n"; return;
        }
        if (auto s = dynamic_cast<StringExpr*>(e)) {
            tab(); std::cout << "String(\"" << s->value << "\")\n"; return;
        }
        if (auto v = dynamic_cast<VarExpr*>(e)) {
            tab(); std::cout << "Var(" << v->name << ")\n"; return;
        }
        if (auto f = dynamic_cast<FuncCallExpr*>(e)) {
            tab(); std::cout << "FuncCall(" << f->name << ")\n";
            indent++;
            for (auto &arg : f->args) printExpr(arg.get());
            indent--;
            return;
        }
        if (auto u = dynamic_cast<UnaryExpr*>(e)) {
            tab(); std::cout << "Unary(" << u->op << ")\n";
            indent++; printExpr(u->expr.get()); indent--;
            return;
        }
        if (auto b2 = dynamic_cast<BinaryExpr*>(e)) {
            tab(); std::cout << "Binary(" << b2->op << ")\n";
            indent++;
            printExpr(b2->left.get());
            printExpr(b2->right.get());
            indent--;
            return;
        }

        tab(); std::cout << "(unknown expr)\n";
    }

    // ==========================
    // Blocks
    // ==========================
    void printBlock(BlockStmt* b) {
        tab(); std::cout << "{\n";
        indent++;
        for (auto &s : b->stmts) printStmt(s.get());
        indent--;
        tab(); std::cout << "}\n";
    }

    // ==========================
    // Statements
    // ==========================
    void printStmt(Stmt* s) {
        if (!s) return;

        if (auto v = dynamic_cast<VarDeclStmt*>(s)) {
            tab(); std::cout << "VarDecl(" << v->type << " " << v->name << ")\n";
            if (v->init) { indent++; printExpr(v->init.get()); indent--; }
            return;
        }

        if (auto a = dynamic_cast<AssignStmt*>(s)) {
            tab(); std::cout << "Assign(" << a->name << ")\n";
            indent++; printExpr(a->value.get()); indent--;
            return;
        }

        if (auto p = dynamic_cast<PrintStmt*>(s)) {
            tab(); std::cout << "Print\n";
            indent++; printExpr(p->expr.get()); indent--;
            return;
        }

            if (auto fcs = dynamic_cast<FuncCallStmt*>(s)) {
                tab(); std::cout << "FuncCallStmt(" << fcs->name << ")\n";
                indent++;
                for (auto &arg : fcs->args) printExpr(arg.get());
                indent--;
                return;
            }

        if (auto i = dynamic_cast<InputStmt*>(s)) {
            tab(); std::cout << "Input(" << i->name << ")\n";
            return;
        }

        if (dynamic_cast<NewlineStmt*>(s)) {
            tab(); std::cout << "Newline\n";
            return;
        }

        if (auto r = dynamic_cast<ReturnStmt*>(s)) {
            tab(); std::cout << "Return\n";
            if (r->value) { indent++; printExpr(r->value.get()); indent--; }
            return;
        }

        if (auto f = dynamic_cast<ForStmt*>(s)) {
            tab(); std::cout << "For(" << f->var << ")\n";
            indent++;
            printExpr(f->start.get());
            printExpr(f->end.get());
            printBlock(f->block.get());
            indent--;
            return;
        }

        if (auto w = dynamic_cast<WhileStmt*>(s)) {
            tab(); std::cout << "While\n";
            indent++; 
                printExpr(w->cond.get());
                printBlock(w->block.get());
            indent--;
            return;
        }

        if (auto ifs = dynamic_cast<IfStmt*>(s)) {
            tab(); std::cout << "If\n";
            indent++; printExpr(ifs->cond.get()); indent--;

            tab(); std::cout << "Then:\n";
            indent++; printBlock(ifs->then_block.get()); indent--;

            if (ifs->else_block) {
                tab(); std::cout << "Else:\n";
                indent++; printBlock(ifs->else_block.get()); indent--;
            }
            return;
        }

        if (auto b = dynamic_cast<BlockStmt*>(s)) {
            printBlock(b);
            return;
        }

        tab(); std::cout << "(unknown stmt)\n";
    }

    // ==========================
    // Functions
    // ==========================
    void printFunc(FuncDecl* f) {
        tab();
        std::cout << "FuncDecl(" << f->name << ")\n";

        indent++;

        if (!f->params.empty()) {
            tab(); std::cout << "Params:\n";
            indent++;
            for (auto &p : f->params) {
                tab(); std::cout << p.type << " " << p.name << "\n";
            }
            indent--;
        }

        tab(); std::cout << "Body:\n";
        indent++;
        printBlock(f->body.get());
        indent--;

        indent--;
    }

public:
    // ==========================
    // Program
    // ==========================
    void print(Program* p) {
        std::cout << "=== AST ===\n";

        for (auto &d : p->decls) {

            if (auto f = dynamic_cast<FuncDecl*>(d.get())) {
                printFunc(f);
                continue;
            }

            if (auto v = dynamic_cast<VarDeclStmt*>(d.get())) {
                printStmt(v);
                continue;
            }

            if (auto s = dynamic_cast<Stmt*>(d.get())) {
                printStmt(s);
                continue;
            }

            std::cout << "(Unknown declaration)\n";
        }

        std::cout << "=== END AST ===\n";
    }
};

#endif

