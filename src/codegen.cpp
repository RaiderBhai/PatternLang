#include "codegen.h"
#include <sstream>
#include <iostream>
#include <set>
#include <cctype>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
using namespace std;

static std::string cppTypeFor(const std::string &t) {
    if (t == "int") return "int";
    if (t == "bool") return "bool";
    if (t == "string") return "std::string";
    return "auto";
}

void CodeGenerator::collectVarTypes(Program* program) {
    varTypes.clear();
    // scan program declarations for VarDeclStmt and FuncDecl params
    for (auto &d : program->decls) {
        if (auto v = dynamic_cast<VarDeclStmt*>(d.get())) {
            varTypes[v->name] = v->type;
        } else if (auto f = dynamic_cast<FuncDecl*>(d.get())) {
            // parameters -> record these so we know types for param names
            for (auto &p : f->params) varTypes[p.name] = p.type;
        }
    }
}

bool CodeGenerator::isQuotedString(const string &s) const {
    return s.size() >= 2 && s.front() == '"' && s.back() == '"';
}
bool CodeGenerator::isBoolLiteral(const string &s) const {
    return s == "true" || s == "false";
}
bool CodeGenerator::isIntLiteral(const string &s) const {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-') { if (s.size() == 1) return false; i = 1; }
    for (; i < s.size(); ++i) if (!isdigit((unsigned char)s[i])) return false;
    return true;
}
bool CodeGenerator::isTemp(const string &s) const {
    return !s.empty() && s[0] == 't' && s.size() > 1 && isdigit((unsigned char)s[1]);
}
bool CodeGenerator::isLiteral(const string &s) const {
    return isQuotedString(s) || isBoolLiteral(s) || isIntLiteral(s);
}

std::string CodeGenerator::makeExprOperand(const std::string &op) const {
    // literals stay as-is; identifiers/temps are used directly
    if (isQuotedString(op)) return op;
    if (isBoolLiteral(op)) return op;
    return op;
}

// Decide a type for a temp by inspecting its uses between startIdx and endIdx (inclusive start, exclusive end)
std::string CodeGenerator::decideTempType(const std::string &temp,
                                         const TACList &tac,
                                         size_t startIdx, size_t endIdx) const
{
    bool seenString = false;
    bool seenBool = false;
    bool seenInt = false;
    // helper: test a token usage for type hints
    auto analyzeToken = [&](const std::string &tok) {
        if (tok.empty()) return;
        if (isQuotedString(tok)) seenString = true;
        else if (isBoolLiteral(tok)) seenBool = true;
        else if (isIntLiteral(tok)) seenInt = true;
        else if (isTemp(tok)) {
            // can't conclude from other temps here
        } else {
            auto it = varTypes.find(tok);
            if (it != varTypes.end()) {
                if (it->second == "string") seenString = true;
                else if (it->second == "bool") seenBool = true;
                else if (it->second == "int") seenInt = true;
            } else {
                seenInt = true; // conservative default
            }
        }
    };
    for (size_t i = startIdx; i < endIdx && i < tac.size(); ++i) {
        const TAC &t = tac[i];
        if (t.res == temp || t.arg1 == temp || t.arg2 == temp) {
            static const std::unordered_set<std::string> boolOps = {
                "<", ">", "<=", ">=", "==", "!=", "&&", "||", "!"
            };
            if (boolOps.count(t.op)) {
                seenBool = true;
            }
            analyzeToken(t.arg1);
            analyzeToken(t.arg2);
            if (t.op == "assign" && t.arg1.size()) analyzeToken(t.arg1);
        }
    }
    if (seenString) return "std::string";
    if (seenBool) return "bool";
    return "int";
}

// Find AST FuncDecl by name (or nullptr)
static FuncDecl* findFuncDecl(Program* program, const std::string &name) {
    for (auto &d : program->decls) {
        if (auto f = dynamic_cast<FuncDecl*>(d.get())) {
            if (f->name == name) return f;
        }
    }
    return nullptr;
}

std::string CodeGenerator::generate(const TACList &tac, Program* program) {
    collectVarTypes(program);
    declared.clear();
    ostringstream out;

    out << "#include <iostream>\n";
    out << "#include <string>\n";
    out << "#include <cmath>\n";
    out << "using namespace std;\n\n";

    // Built-in pattern and math functions: emit only those that are actually used
    std::unordered_set<std::string> usedBuiltins;
    // scan TAC calls
    for (const auto &t : tac) {
        if (t.op == "call" && !t.arg1.empty()) usedBuiltins.insert(t.arg1);
    }
    // scan top-level AST for direct function calls / prints that might call builtins
    for (auto &nodePtr : program->decls) {
        if (auto fcs = dynamic_cast<FuncCallStmt*>(nodePtr.get())) {
            usedBuiltins.insert(fcs->name);
        } else if (auto ps = dynamic_cast<PrintStmt*>(nodePtr.get())) {
            if (auto call = dynamic_cast<FuncCallExpr*>(ps->expr.get())) usedBuiltins.insert(call->name);
        } else if (auto fcd = dynamic_cast<FuncDecl*>(nodePtr.get())) {
            // scan body-less: skip (functions will be handled from TAC)
        }
    }

    // mapping of builtin name -> implementation
    const std::unordered_map<std::string, std::string> builtinCode = {
        {"repeat", "string repeat(string c, int times) { string s; for (int i = 0; i < times; ++i) s += c; return s; }\n"},
        {"pyramid", "void pyramid(int height) { for (int i = 1; i <= height; ++i) { for (int j = 0; j < height - i; ++j) cout << ' '; for (int j = 0; j < 2 * i - 1; ++j) cout << '*'; cout << endl; } }\n"},
        {"diamond", "void diamond(int height) { int n = height; for (int i = 1; i <= n; ++i) { for (int j = 0; j < n - i; ++j) cout << ' '; for (int j = 0; j < 2 * i - 1; ++j) cout << '*'; cout << endl; } for (int i = n - 1; i >= 1; --i) { for (int j = 0; j < n - i; ++j) cout << ' '; for (int j = 0; j < 2 * i - 1; ++j) cout << '*'; cout << endl; } }\n"},
        {"line", "void line(string c, int width) { for (int i = 0; i < width; ++i) cout << c; cout << endl; }\n"},
        {"box", "void box(string c, int width, int height) { for (int i = 0; i < height; ++i) { for (int j = 0; j < width; ++j) cout << c; cout << endl; } }\n"},
        {"stairs", "void stairs(int height, string c) { for (int i = 1; i <= height; ++i) { for (int j = 0; j < i; ++j) cout << c; cout << endl; } }\n"},
        {"max", "int max(int a, int b) { return a > b ? a : b; }\n"},
        {"min", "int min(int a, int b) { return a < b ? a : b; }\n"},
        {"abs", "int abs(int x) { return x < 0 ? -x : x; }\n"},
        {"pow", "int pow(int a, int b) { return static_cast<int>(std::pow(a, b)); }\n"},
        {"sqrt", "int sqrt(int n) { return static_cast<int>(std::sqrt(n)); }\n"},
        {"rangeSum", "int rangeSum(int n) { int s = 0; for (int i = 1; i <= n; ++i) s += i; return s; }\n"},
        {"factor", "void factor(int n) { for (int i = 2; i <= n; ++i) { while (n % i == 0) { cout << i << ' '; n /= i; } } cout << endl; }\n"},
        {"isPrime", "bool isPrime(int n) { if (n <= 1) return false; for (int i = 2; i * i <= n; ++i) if (n % i == 0) return false; return true; }\n"},
        {"table", "void table(int n) { for (int i = 1; i <= n; ++i) { for (int j = 1; j <= n; ++j) cout << i * j << '\t'; cout << endl; } }\n"},
        {"patternMultiply", "void patternMultiply(int a, int b) { for (int i = 0; i < a; ++i) { for (int j = 0; j < b; ++j) cout << '*'; cout << endl; } }\n"}
    };

    // Emit only used builtins, and avoid emitting if user defined a function with same name
    for (const auto &kv : builtinCode) {
        const std::string &name = kv.first;
        if (usedBuiltins.count(name)) {
            if (!findFuncDecl(program, name)) {
                out << kv.second;
            }
        }
    }
    out << "\n";

    // find function label indices ("func_<name>")
    std::unordered_map<std::string, size_t> funcLabelIndex;
    for (size_t i = 0; i < tac.size(); ++i) {
        if (tac[i].isLabel) {
            string lab = tac[i].res;
            if (lab.rfind("func_", 0) == 0) {
                string fname = lab.substr(5);
                funcLabelIndex[fname] = i;
            }
        }
    }

    // Compute function ranges (start, end) with preference:
    // 1) If there exists an explicit "endfunc_<name>" label, use that label index+1 as end.
    // 2) Else search for the first 'return' within the function and stop after it.
    // 3) Else stop at next func_ label or end-of-TAC.
    std::unordered_map<std::string, pair<size_t,size_t>> funcRanges;
    for (auto &fp : funcLabelIndex) {
        const string &fname = fp.first;
        size_t start = fp.second + 1;
        size_t end = start;
        // try to find explicit endfunc_<name> label
        string endLabel = "endfunc_" + fname;
        bool foundEndLabel = false;
        for (size_t j = start; j < tac.size(); ++j) {
            if (tac[j].isLabel && tac[j].res == endLabel) {
                end = j + 1; // include the end label index so that range stops after it
                foundEndLabel = true;
                break;
            }
        }
        if (!foundEndLabel) {
            // fallback: find first return or next func_ label
            size_t j = start;
            for (; j < tac.size(); ++j) {
                if (tac[j].isLabel && tac[j].res.rfind("func_", 0) == 0) {
                    // next function starts — stop before it
                    break;
                }
                if (tac[j].op == "return") {
                    j += 1; // include the return and stop
                    break;
                }
            }
            end = j;
        }
        funcRanges[fname] = { start, end };
    }

    // forward declarations for functions with correct parameter lists
    for (auto &p : funcLabelIndex) {
        FuncDecl* fdecl = findFuncDecl(program, p.first);
        out << "int " << p.first << "(";
        if (fdecl && !fdecl->params.empty()) {
            for (size_t pi = 0; pi < fdecl->params.size(); ++pi) {
                auto &param = fdecl->params[pi];
                out << cppTypeFor(param.type) << " " << param.name;
                if (pi + 1 < fdecl->params.size()) out << ", ";
            }
        }
        out << ");\n";
    }
    out << "\n";

    // Helper to collect temps used in [startIdx, endIdx)
    auto collectTempsRange = [&](size_t startIdx, size_t endIdx) {
        std::unordered_set<std::string> temps;
        for (size_t i = startIdx; i < endIdx && i < tac.size(); ++i) {
            const TAC &t = tac[i];
            if (!t.res.empty() && isTemp(t.res)) temps.insert(t.res);
            if (!t.arg1.empty() && isTemp(t.arg1)) temps.insert(t.arg1);
            if (!t.arg2.empty() && isTemp(t.arg2)) temps.insert(t.arg2);
        }
        return temps;
    };

    // Emit function definitions using the computed ranges
    for (auto &fp : funcLabelIndex) {
        string fname = fp.first;
        auto rngIt = funcRanges.find(fname);
        size_t funcStart = (rngIt != funcRanges.end()) ? rngIt->second.first : fp.second + 1;
        size_t funcEnd = (rngIt != funcRanges.end()) ? rngIt->second.second : funcStart;

        // find AST FuncDecl to get parameters
        FuncDecl* fdecl = findFuncDecl(program, fname);

        // collect temps in this function range and decide types
        auto temps = collectTempsRange(funcStart, funcEnd);
        std::vector<std::pair<std::string,std::string>> tempDecls;
        for (auto &tname : temps) {
            string ttype = decideTempType(tname, tac, funcStart, funcEnd);
            tempDecls.emplace_back(tname, ttype);
        }

        // emit function signature with params (if available)
        out << "int " << fname << "(";
        if (fdecl && !fdecl->params.empty()) {
            for (size_t pi = 0; pi < fdecl->params.size(); ++pi) {
                auto &p = fdecl->params[pi];
                out << cppTypeFor(p.type) << " " << p.name;
                if (pi + 1 < fdecl->params.size()) out << ", ";
            }
        }
        out << ") {\n";

        // declare temps at top of function
        for (auto &p : tempDecls) {
            out << "    " << p.second << " " << p.first << " = ";
            if (p.second == "std::string") out << "\"\";\n";
            else if (p.second == "bool") out << "false;\n";
            else out << "0;\n";
        }

        // function-local declared map
        declared.clear();
        // mark function parameters as declared so they aren't redeclared
        if (fdecl) {
            for (auto &param : fdecl->params) declared[param.name] = true;
        }

        // emit body (only TAC within function range)
        for (size_t k = funcStart; k < funcEnd && k < tac.size(); ++k) {
            const TAC &t = tac[k];
            // skip function labels like "func_test", but emit normal labels like L1, L2
            if (t.isLabel) {
                if (t.res.rfind("func_", 0) == 0) continue;  // skip function label
                out << "    " << t.res << ":\n";             // <-- emit labels inside functions
                continue;
            }

            if (t.op == "assign") {
                string dst = t.res;
                string src = makeExprOperand(t.arg1);
                if (isTemp(dst)) {
                    out << "    " << dst << " = " << src << ";\n";
                } else {
                    if (!declared[dst]) {
                        string ty = "int";
                        if (varTypes.find(dst) != varTypes.end()) ty = cppTypeFor(varTypes[dst]);
                        out << "    " << ty << " " << dst << " = " << src << ";\n";
                        declared[dst] = true;
                    } else {
                        out << "    " << dst << " = " << src << ";\n";
                    }
                }
            }
            else if (t.op == "print") {
                out << "    cout << " << makeExprOperand(t.arg1) << ";\n";
            }
            else if (t.op == "newline") {
                out << "    cout << endl;\n";
            }
            else if (t.op == "call") {
                string dst = t.res;
                string args = t.arg2;
                if (dst.empty()) {
                    // call as statement
                    out << "    " << t.arg1 << "(" << args << ");\n";
                } else if (isTemp(dst)) {
                    out << "    " << dst << " = " << t.arg1 << "(" << args << ");\n";
                } else {
                    if (!declared[dst]) {
                        string ty = "int";
                        if (varTypes.find(dst) != varTypes.end()) ty = cppTypeFor(varTypes[dst]);
                        out << "    " << ty << " " << dst << " = " << t.arg1 << "(" << args << ");\n";
                        declared[dst] = true;
                    } else {
                        out << "    " << dst << " = " << t.arg1 << "(" << args << ");\n";
                    }
                }
            }
            else if (t.op == "return") {
                if (t.arg1.empty()) out << "    return 0;\n";
                else out << "    return " << makeExprOperand(t.arg1) << ";\n";
            }
            else if (t.op == "goto") {
                out << "    goto " << t.res << ";\n";
            }
            else if (t.op == "ifFalse") {
                out << "    if (!(" << makeExprOperand(t.arg1) << ")) goto " << t.res << ";\n";
            }
            else {
                // binary/unary ops
                string dst = t.res;
                string a1 = makeExprOperand(t.arg1);
                if (!t.arg2.empty()) {
                    string a2 = makeExprOperand(t.arg2);
                    if (isTemp(dst)) {
                        out << "    " << dst << " = " << a1 << " " << t.op << " " << a2 << ";\n";
                    } else {
                        if (!declared[dst]) {
                            string ty = "int";
                            if (varTypes.find(dst) != varTypes.end()) ty = cppTypeFor(varTypes[dst]);
                            out << "    " << ty << " " << dst << " = " << a1 << " " << t.op << " " << a2 << ";\n";
                            declared[dst] = true;
                        } else {
                            out << "    " << dst << " = " << a1 << " " << t.op << " " << a2 << ";\n";
                        }
                    }
                } else {
                    // unary
                    if (isTemp(dst)) {
                        out << "    " << dst << " = " << t.op << " " << a1 << ";\n";
                    } else {
                        if (!declared[dst]) {
                            string ty = "int";
                            if (varTypes.find(dst) != varTypes.end()) ty = cppTypeFor(varTypes[dst]);
                            out << "    " << ty << " " << dst << " = " << t.op << " " << a1 << ";\n";
                            declared[dst] = true;
                        } else {
                            out << "    " << dst << " = " << t.op << " " << a1 << ";\n";
                        }
                    }
                }
            }
        } // end function body
        out << "    return 0;\n";
        out << "}\n\n";
    } // end functions

    // --- MAIN ---
    declared.clear();

    out << "int main() {\n";
    // Pre-declare named top-level vars (from varTypes) in main with default init
    for (auto &pair : varTypes) {
        out << "    " << cppTypeFor(pair.second) << " " << pair.first << " = ";
        if (pair.second == "int") out << "0;\n";
        else if (pair.second == "bool") out << "false;\n";
        else out << "\"\";\n";
        declared[pair.first] = true;
    }

    // Collect temps used in top-level TAC (not inside functions) and declare them at top
    std::unordered_set<std::string> mainTemps;
    bool topLevelHasCallsOrPrints = false;
    for (size_t i = 0; i < tac.size(); ++i) {
        bool inside = false;
        for (auto &fr : funcRanges) {
            if (i >= fr.second.first && i < fr.second.second) { inside = true; break; }
        }
        if (inside) continue;
        const TAC &t = tac[i];
        if (t.op == "call" || t.op == "print") topLevelHasCallsOrPrints = true;
        if (!t.res.empty() && isTemp(t.res)) mainTemps.insert(t.res);
        if (!t.arg1.empty() && isTemp(t.arg1)) mainTemps.insert(t.arg1);
        if (!t.arg2.empty() && isTemp(t.arg2)) mainTemps.insert(t.arg2);
    }
    for (auto &mt : mainTemps) {
        std::string ty = decideTempType(mt, tac, 0, tac.size());
        out << "    " << ty << " " << mt << " = ";
        if (ty == "std::string") out << "\"\";\n";
        else if (ty == "bool") out << "false;\n";
        else out << "0;\n";
        declared[mt] = true;
    }

    // Emit top-level code by translating TAC entries that are not inside any function ranges.
    // Build a boolean mask for indices that belong to functions.
    std::vector<bool> inFunction(tac.size(), false);
    for (auto &fr : funcRanges) {
        size_t s = fr.second.first;
        size_t e = fr.second.second;
        for (size_t i = s; i < e && i < tac.size(); ++i) inFunction[i] = true;
    }

    for (size_t k = 0; k < tac.size(); ++k) {
        if (inFunction[k]) continue;
        const TAC &t = tac[k];

        if (t.isLabel) {
            // top-level labels can be emitted as labels in main
            out << "    " << t.res << ":\n";
            continue;
        }

        if (t.op == "assign") {
            string dst = t.res;
            string src = makeExprOperand(t.arg1);
            if (!declared[dst]) {
                string ty = "int";
                if (varTypes.find(dst) != varTypes.end()) ty = cppTypeFor(varTypes[dst]);
                out << "    " << ty << " " << dst << " = " << src << ";\n";
                declared[dst] = true;
            } else {
                out << "    " << dst << " = " << src << ";\n";
            }
        }
        else if (t.op == "print") {
            out << "    cout << " << makeExprOperand(t.arg1) << " << endl;\n";
        }
        else if (t.op == "newline") {
            out << "    cout << endl;\n";
        }
        else if (t.op == "input") {
            out << "    cin >> " << t.arg1 << ";\n";
        }
        else if (t.op == "call") {
            // t.arg1 = func name, t.arg2 = args string, t.res = destination (maybe temp/var) or empty for statement-call
            if (t.res.empty()) {
                out << "    " << t.arg1 << "(" << t.arg2 << ");\n";
            } else if (isTemp(t.res)) {
                out << "    " << t.res << " = " << t.arg1 << "(" << t.arg2 << ");\n";
            } else {
                if (!declared[t.res]) {
                    string ty = "int";
                    if (varTypes.find(t.res) != varTypes.end()) ty = cppTypeFor(varTypes[t.res]);
                    out << "    " << ty << " " << t.res << " = " << t.arg1 << "(" << t.arg2 << ");\n";
                    declared[t.res] = true;
                } else {
                    out << "    " << t.res << " = " << t.arg1 << "(" << t.arg2 << ");\n";
                }
            }
        }
        else if (t.op == "goto") {
            out << "    goto " << t.res << ";\n";
        }
        else if (t.op == "ifFalse") {
            out << "    if (!(" << makeExprOperand(t.arg1) << ")) goto " << t.res << ";\n";
        }
        else if (t.op == "return") {
            if (t.arg1.empty()) out << "    return 0;\n";
            else out << "    return " << makeExprOperand(t.arg1) << ";\n";
        }
        else {
            // other binary/unary ops
            string dst = t.res;
            string a1 = makeExprOperand(t.arg1);
            if (!t.arg2.empty()) {
                string a2 = makeExprOperand(t.arg2);
                if (!declared[dst]) {
                    string ty = "int";
                    if (varTypes.find(dst) != varTypes.end()) ty = cppTypeFor(varTypes[dst]);
                    out << "    " << ty << " " << dst << " = " << a1 << " " << t.op << " " << a2 << ";\n";
                    declared[dst] = true;
                } else {
                    out << "    " << dst << " = " << a1 << " " << t.op << " " << a2 << ";\n";
                }
            } else {
                if (!declared[dst]) {
                    string ty = "int";
                    if (varTypes.find(dst) != varTypes.end()) ty = cppTypeFor(varTypes[dst]);
                    out << "    " << ty << " " << dst << " = " << t.op << " " << a1 << ";\n";
                    declared[dst] = true;
                } else {
                    out << "    " << dst << " = " << t.op << " " << a1 << ";\n";
                }
            }
        }
    }

    // If top-level TAC didn't contain calls/prints, fallback to emitting top-level AST nodes
    if (!topLevelHasCallsOrPrints) {
        for (auto &nodePtr : program->decls) {
            if (auto fcs = dynamic_cast<FuncCallStmt*>(nodePtr.get())) {
                out << "    " << fcs->name << "(";
                for (size_t i = 0; i < fcs->args.size(); ++i) {
                    auto &arg = fcs->args[i];
                    if (auto num = dynamic_cast<NumberExpr*>(arg.get())) out << num->value;
                    else if (auto str = dynamic_cast<StringExpr*>(arg.get())) out << '"' << str->value << '"';
                    else if (auto b = dynamic_cast<BoolExpr*>(arg.get())) out << b->value;
                    else if (auto v = dynamic_cast<VarExpr*>(arg.get())) out << v->name;
                    else if (auto unary = dynamic_cast<UnaryExpr*>(arg.get())) {
                        if (unary->op == "-") {
                            if (auto num = dynamic_cast<NumberExpr*>(unary->expr.get())) out << "-" << num->value;
                        }
                    }
                    if (i + 1 < fcs->args.size()) out << ", ";
                }
                out << ");\n";
            } else if (auto ps = dynamic_cast<PrintStmt*>(nodePtr.get())) {
                out << "    cout << ";
                auto &expr = ps->expr;
                if (auto call = dynamic_cast<FuncCallExpr*>(expr.get())) {
                    out << call->name << "(";
                    for (size_t i = 0; i < call->args.size(); ++i) {
                        auto &arg = call->args[i];
                        if (auto num = dynamic_cast<NumberExpr*>(arg.get())) out << num->value;
                        else if (auto str = dynamic_cast<StringExpr*>(arg.get())) out << '"' << str->value << '"';
                        else if (auto b = dynamic_cast<BoolExpr*>(arg.get())) out << b->value;
                        else if (auto v = dynamic_cast<VarExpr*>(arg.get())) out << v->name;
                        else if (auto unary = dynamic_cast<UnaryExpr*>(arg.get())) {
                            if (unary->op == "-") {
                                if (auto num = dynamic_cast<NumberExpr*>(unary->expr.get())) out << "-" << num->value;
                            }
                        }
                        if (i + 1 < call->args.size()) out << ", ";
                    }
                    out << ")";
                } else if (auto num = dynamic_cast<NumberExpr*>(expr.get())) {
                    out << num->value;
                } else if (auto str = dynamic_cast<StringExpr*>(expr.get())) {
                    out << '"' << str->value << '"';
                } else if (auto b = dynamic_cast<BoolExpr*>(expr.get())) {
                    out << b->value;
                } else if (auto v = dynamic_cast<VarExpr*>(expr.get())) {
                    out << v->name;
                }
                out << " << endl;\n";
            } else if (dynamic_cast<NewlineStmt*>(nodePtr.get())) {
                out << "    cout << endl;\n";
            } else if (auto ins = dynamic_cast<InputStmt*>(nodePtr.get())) {
                out << "    cin >> " << ins->name << ";\n";
            }
        }
    }

    out << "    return 0;\n";
    out << "}\n";
    return out.str();
}