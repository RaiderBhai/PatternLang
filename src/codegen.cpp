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
    out << "using namespace std;\n\n";

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
                if (isTemp(dst)) {
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

    // pre-declare named top-level vars (from varTypes) in main with default init
    for (auto &pair : varTypes) {
        out << "    " << cppTypeFor(pair.second) << " " << pair.first << " = ";
        if (pair.second == "int") out << "0;\n";
        else if (pair.second == "bool") out << "false;\n";
        else out << "\"\";\n";
        declared[pair.first] = true;
    }

    // build vector of function ranges (from funcRanges) so we can skip them when emitting main body
    std::vector<pair<size_t,size_t>> funcRangesVec;
    for (auto &fr : funcRanges) funcRangesVec.push_back(fr.second);
    auto inFuncRange = [&](size_t idx)->bool{
        for (auto &r : funcRangesVec) if (idx >= r.first && idx < r.second) return true;
        return false;
    };

    // collect temps used in main (indices not part of any funcRange)
    std::unordered_set<std::string> mainTemps;
    for (size_t i = 0; i < tac.size(); ++i) {
        if (inFuncRange(i)) continue;
        const TAC &t = tac[i];
        if (!t.res.empty() && isTemp(t.res)) mainTemps.insert(t.res);
        if (!t.arg1.empty() && isTemp(t.arg1)) mainTemps.insert(t.arg1);
        if (!t.arg2.empty() && isTemp(t.arg2)) mainTemps.insert(t.arg2);
    }

    // decide types for main temps and emit declarations at top
    std::vector<pair<string,string>> mainTempDecls;
    for (auto &tname : mainTemps) {
        string ttype = decideTempType(tname, tac, 0, tac.size());
        mainTempDecls.emplace_back(tname, ttype);
    }
    for (auto &p : mainTempDecls) {
        out << "    " << p.second << " " << p.first << " = ";
        if (p.second == "std::string") out << "\"\";\n";
        else if (p.second == "bool") out << "false;\n";
        else out << "0;\n";
    }

    // Now emit TAC that is not part of functions
    for (size_t i = 0; i < tac.size(); ++i) {
        if (inFuncRange(i)) continue;
        const TAC &t = tac[i];
        if (t.isLabel) {
            // non-function label -> emit label
            if (t.res.rfind("func_",0) == 0) continue;
            // also skip explicit endfunc_ labels (they are not meaningful in C++ output)
            if (t.res.rfind("endfunc_",0) == 0) continue;
            out << t.res << ":\n";
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
            // fix: if args are comma separated already, use as is. If empty, call without args.
            if (isTemp(dst)) {
                if (args.empty())
                    out << "    " << dst << " = " << t.arg1 << "();\n";
                else
                    out << "    " << dst << " = " << t.arg1 << "(" << args << ");\n";
            } else {
                if (!declared[dst]) {
                    string ty = "int";
                    if (varTypes.find(dst) != varTypes.end()) ty = cppTypeFor(varTypes[dst]);
                    if (args.empty())
                        out << "    " << ty << " " << dst << " = " << t.arg1 << "();\n";
                    else
                        out << "    " << ty << " " << dst << " = " << t.arg1 << "(" << args << ");\n";
                    declared[dst] = true;
                } else {
                    if (args.empty())
                        out << "    " << dst << " = " << t.arg1 << "();\n";
                    else
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
            // binary / unary ops
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
    } // end main body

    out << "    return 0;\n";
    out << "}\n";
    return out.str();
}
