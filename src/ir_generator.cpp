#include "ir_generator.h"
#include <sstream>
#include <iostream>

// ---------- constructors ----------
IRGenerator::IRGenerator() : tmpCount(0), labelCount(0) {}

// ---------- helpers ----------
std::string IRGenerator::newTemp() {
    return "t" + std::to_string(++tmpCount);
}
std::string IRGenerator::newLabel(const std::string &base) {
    return base + std::to_string(++labelCount);
}
void IRGenerator::emitAssign(const std::string &dst, const std::string &src) {
    TAC t; t.op = "assign"; t.arg1 = src; t.res = dst; emit(t);
}

// ---------- main generate ----------
void IRGenerator::generate(Program* program) {
    code.clear();
    tmpCount = 0;
    labelCount = 0;
    // generate functions first (we emit a label for function start)
    for (auto &n : program->decls) {
        if (auto f = dynamic_cast<FuncDecl*>(n.get())) {
            genFunc(f);
        }
    }
    // globals and top-level statements
    for (auto &n : program->decls) {
        if (auto v = dynamic_cast<VarDeclStmt*>(n.get())) {
            // if initializer exists, emit assignment to global var
            if (v->init) {
                std::string rhs = genExpr(v->init.get());
                emitAssign(v->name, rhs);
            } else {
                // default init depending on type: emit assignment to 0/false/""
                if (v->type == "int") emitAssign(v->name, "0");
                else if (v->type == "bool") emitAssign(v->name, "false");
                else emitAssign(v->name, "\"\"");
            }
        } else if (auto s = dynamic_cast<Stmt*>(n.get())) {
            genStmt(s);
        }
    }
}

// ---------- expressions ----------
std::string IRGenerator::genExpr(Expr* e) {
    if (!e) return "0";
    if (auto n = dynamic_cast<NumberExpr*>(e)) {
        return n->value;
    }
    if (auto b = dynamic_cast<BoolExpr*>(e)) {
        return b->value == "true" ? "true" : "false";
    }
    if (auto s = dynamic_cast<StringExpr*>(e)) {
        // keep quotes in literal
        return std::string("\"") + s->value + "\"";
    }
    if (auto v = dynamic_cast<VarExpr*>(e)) {
        return v->name;
    }
    if (auto call = dynamic_cast<FuncCallExpr*>(e)) {
        // evaluate args
        std::string argList;
        for (size_t i = 0; i < call->args.size(); ++i) {
            std::string a = genExpr(call->args[i].get());
            if (i == 0) argList = a;
            else argList += ", " + a;
        }
        std::string tmp = newTemp();
        TAC t; t.op = "call"; t.arg1 = call->name; t.arg2 = argList; t.res = tmp;
        emit(t);
        return tmp;
    }
    if (auto u = dynamic_cast<UnaryExpr*>(e)) {
        std::string sub = genExpr(u->expr.get());
        std::string tmp = newTemp();
        TAC t; t.op = u->op; t.arg1 = sub; t.res = tmp;
        emit(t);
        return tmp;
    }
    if (auto bin = dynamic_cast<BinaryExpr*>(e)) {
        std::string L = genExpr(bin->left.get());
        std::string R = genExpr(bin->right.get());
        std::string tmp = newTemp();
        TAC t; t.op = bin->op; t.arg1 = L; t.arg2 = R; t.res = tmp;
        emit(t);
        return tmp;
    }
    return "0";
}

// ---------- statements ----------
void IRGenerator::genStmt(Stmt* s) {
    if (!s) return;
    if (auto v = dynamic_cast<VarDeclStmt*>(s)) {
        if (v->init) {
            std::string rhs = genExpr(v->init.get());
            emitAssign(v->name, rhs);
        } else {
            if (v->type == "int") emitAssign(v->name, "0");
            else if (v->type == "bool") emitAssign(v->name, "false");
            else emitAssign(v->name, "\"\"");
        }
        return;
    }
    if (auto a = dynamic_cast<AssignStmt*>(s)) {
        std::string rhs = genExpr(a->value.get());
        emitAssign(a->name, rhs);
        return;
    }
    if (auto p = dynamic_cast<PrintStmt*>(s)) {
        std::string v = genExpr(p->expr.get());
        TAC t; t.op = "print"; t.arg1 = v; emit(t);
        return;
    }
    if (dynamic_cast<NewlineStmt*>(s)) {
        TAC t; t.op = "newline"; emit(t);
        return;
    }
    if (auto r = dynamic_cast<ReturnStmt*>(s)) {
        if (r->value) {
            std::string v = genExpr(r->value.get());
            TAC t; t.op = "return"; t.arg1 = v; emit(t);
        } else {
            TAC t; t.op = "return"; emit(t);
        }
        return;
    }
    if (auto f = dynamic_cast<ForStmt*>(s)) {
        // for i = start to end { block }
        // i = start
        std::string sstart = genExpr(f->start.get());
        emitAssign(f->var, sstart);
        std::string Lbegin = newLabel("L");
        std::string Lend = newLabel("L");
        TAC tl; tl.isLabel = true; tl.res = Lbegin; emit(tl);
        // cond: t = i <= end
        std::string tend = genExpr(f->end.get());
        std::string tcond = newTemp();
        TAC tc; tc.op = "<="; tc.arg1 = f->var; tc.arg2 = tend; tc.res = tcond; emit(tc);
        TAC tif; tif.op = "ifFalse"; tif.arg1 = tcond; tif.res = Lend; emit(tif);
        // body
        genBlock(f->block.get());
        // i = i + 1
        std::string tadd = newTemp();
        TAC tplus; tplus.op = "+"; tplus.arg1 = f->var; tplus.arg2 = "1"; tplus.res = tadd; emit(tplus);
        emitAssign(f->var, tadd);
        // goto begin
        TAC tg; tg.op = "goto"; tg.res = Lbegin; emit(tg);
        TAC tlend; tlend.isLabel = true; tlend.res = Lend; emit(tlend);
        return;
    }
    if (auto w = dynamic_cast<WhileStmt*>(s)) {
        std::string Lbegin = newLabel("L");
        std::string Lend = newLabel("L");
        TAC tL; tL.isLabel = true; tL.res = Lbegin; emit(tL);
        std::string cond = genExpr(w->cond.get());
        TAC tif; tif.op = "ifFalse"; tif.arg1 = cond; tif.res = Lend; emit(tif);
        genBlock(w->block.get());
        TAC tg; tg.op = "goto"; tg.res = Lbegin; emit(tg);
        TAC tlend; tlend.isLabel = true; tlend.res = Lend; emit(tlend);
        return;
    }
    if (auto ifs = dynamic_cast<IfStmt*>(s)) {
        std::string Lelse = newLabel("L");
        std::string Lend = newLabel("L");
        std::string cond = genExpr(ifs->cond.get());
        TAC tif; tif.op = "ifFalse"; tif.arg1 = cond; tif.res = ifs->else_block ? Lelse : Lend; emit(tif);
        // then
        genBlock(ifs->then_block.get());
        TAC tg; tg.op = "goto"; tg.res = Lend; emit(tg);
        if (ifs->else_block) {
            TAC tels; tels.isLabel = true; tels.res = Lelse; emit(tels);
            genBlock(ifs->else_block.get());
        }
        TAC tlend; tlend.isLabel = true; tlend.res = Lend; emit(tlend);
        return;
    }
    if (auto b = dynamic_cast<BlockStmt*>(s)) {
        genBlock(b);
        return;
    }
    // unknown statement -> ignore
}

void IRGenerator::genBlock(BlockStmt* b) {
    if (!b) return;
    for (auto &st : b->stmts) genStmt(st.get());
}

// ---------- function generation ----------
void IRGenerator::genFunc(FuncDecl* f) {
    // create a label for function start
    TAC tlabel; tlabel.isLabel = true; tlabel.res = "func_" + f->name; emit(tlabel);

    // parameters: we don't emit special param code here (assume caller will set)
    // but we can optionally add an assign comment for each param (skipped)
    if (f->body) {
        for (auto &s : f->body->stmts) genStmt(s.get());
    }

    // ensure function ends with return (if none emitted)
    TAC tret; tret.op = "return"; emit(tret);

    // **CRITICAL FIX**: emit an explicit end-of-function label so later passes
    // (and code generator) can disambiguate function body from top-level TAC.
    TAC tend; tend.isLabel = true; tend.res = "endfunc_" + f->name; emit(tend);
}
