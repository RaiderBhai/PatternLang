#include "optimizer.h"
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <iostream>

using namespace std;

// ---------- helpers ----------
bool Optimizer::isTemp(const string &s) {
    return !s.empty() && s[0] == 't' && s.size() > 1 && isdigit((unsigned char)s[1]);
}
bool Optimizer::isIntLiteral(const string &s) {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-') { if (s.size() == 1) return false; i = 1; }
    for (; i < s.size(); ++i) if (!isdigit((unsigned char)s[i])) return false;
    return true;
}
bool Optimizer::isBoolLiteral(const string &s) {
    return s == "true" || s == "false";
}
bool Optimizer::isQuotedString(const string &s) {
    return s.size() >= 2 && s.front() == '"' && s.back() == '"';
}
long long Optimizer::toInt(const string &s) {
    try { return stoll(s); } catch(...) { return 0; }
}

// ---------- main optimize (wrapper) ----------
TACList Optimizer::optimize(const TACList &code) {
    // Work on a copy so the input remains const/unchanged
    TACList out = code;

    bool changed = true;
    // Iterate passes until no change (bounded by simple loop)
    int iterations = 0;
    while (changed && iterations < 10) {
        changed = false;
        iterations++;
        changed = constantFold(out) || changed;
        changed = strengthReduce(out) || changed;
        changed = copyPropagate(out) || changed;
        changed = deadCodeElim(out) || changed;
    }

    return out;
}

// ---------- pass implementations ----------
bool Optimizer::constantFold(TACList &code) {
    bool changed = false;
    for (auto &t : code) {
        // only binary ops with two integer literals
        if (t.isLabel) continue;
        if (t.op.empty()) continue;
        // binary op when both args are integer literal or bool literal (for logical ops)
        if (!t.arg2.empty()) {
            if (isIntLiteral(t.arg1) && isIntLiteral(t.arg2)) {
                long long a = toInt(t.arg1);
                long long b = toInt(t.arg2);
                long long res = 0;
                bool fold = true;
                if (t.op == "+") res = a + b;
                else if (t.op == "-") res = a - b;
                else if (t.op == "*") res = a * b;
                else if (t.op == "/") { if (b == 0) fold = false; else res = a / b; }
                else if (t.op == "%") { if (b == 0) fold = false; else res = a % b; }
                else if (t.op == "<") res = (a < b);
                else if (t.op == ">") res = (a > b);
                else if (t.op == "<=") res = (a <= b);
                else if (t.op == ">=") res = (a >= b);
                else if (t.op == "==") res = (a == b);
                else if (t.op == "!=") res = (a != b);
                else fold = false;
                if (fold) {
                    // replace instruction with assign res = <literal>
                    t.op = "assign";
                    t.arg1 = to_string(res);
                    t.arg2.clear();
                    // res remains t.res
                    changed = true;
                }
            } else if (isBoolLiteral(t.arg1) && isBoolLiteral(t.arg2)) {
                // boolean ops: &&, ||, ==, !=
                bool a = (t.arg1 == "true");
                bool b = (t.arg2 == "true");
                if (t.op == "&&" || t.op == "||" || t.op == "==" || t.op == "!=") {
                    bool outv = false;
                    if (t.op == "&&") outv = (a && b);
                    if (t.op == "||") outv = (a || b);
                    if (t.op == "==") outv = (a == b);
                    if (t.op == "!=") outv = (a != b);
                    t.op = "assign";
                    t.arg1 = outv ? "true" : "false";
                    t.arg2.clear();
                    changed = true;
                }
            } else if (isIntLiteral(t.arg1) && isBoolLiteral(t.arg2)) {
                // unlikely combination — skip
            }
        }
    }
    return changed;
}

bool Optimizer::strengthReduce(TACList &code) {
    bool changed = false;
    for (auto &t : code) {
        if (t.isLabel) continue;
        // only binary
        if (t.arg2.empty()) continue;
        if (t.op == "*") {
            // x * 2 => x + x
            if (isIntLiteral(t.arg1) && t.arg1 == "2") {
                // res = arg2 * 2  -> res = arg2 + arg2
                string savedArg2 = t.arg2;
                t.op = "+";
                t.arg1 = savedArg2;
                t.arg2 = savedArg2;
                changed = true;
            } else if (isIntLiteral(t.arg2) && t.arg2 == "2") {
                // res = arg1 * 2 -> res = arg1 + arg1
                t.op = "+";
                t.arg2 = t.arg1;
                changed = true;
            }
        }
        // other strength reductions could be added (like * pow2 -> <<)
    }
    return changed;
}

bool Optimizer::copyPropagate(TACList &code) {
    bool changed = false;
    unordered_map<string, string> repl; // temp -> replacement (literal or other temp)
    // We will do a single left-to-right propagation with updating mapping when assignments occur.
    for (size_t i = 0; i < code.size(); ++i) {
        TAC &t = code[i];
        if (t.isLabel) continue;
        // replace args if mapping exists
        if (!t.arg1.empty() && repl.find(t.arg1) != repl.end()) {
            t.arg1 = repl[t.arg1];
            changed = true;
        }
        if (!t.arg2.empty() && repl.find(t.arg2) != repl.end()) {
            t.arg2 = repl[t.arg2];
            changed = true;
        }
        // if this is a simple assign where result is a temp and arg1 is literal or temp
        if (t.op == "assign" && isTemp(t.res)) {
            // record mapping res -> arg1
            // but only if arg1 is literal or temp (safe)
            if (isIntLiteral(t.arg1) || isBoolLiteral(t.arg1) || isQuotedString(t.arg1) || isTemp(t.arg1)) {
                repl[t.res] = t.arg1;
                // do not remove here; DCE will eliminate unused temps
                changed = true;
            }
        } else {
            // when something defines a temp differently, invalidate mapping for that temp
            if (isTemp(t.res)) {
                repl.erase(t.res);
            }
        }
        // special: op "call" creates a temp in t.res; if it was in repl, remove
        if (t.op == "call" && isTemp(t.res)) repl.erase(t.res);
    }
    // After propagation we might produce assign instructions that assign a temp to a literal (which can be DCE'd)
    return changed;
}

bool Optimizer::deadCodeElim(TACList &code) {
    bool changed = false;
    // Iterative removal of definitions whose result is a temp never used later.
    // Compute used temps: any occurrence in arg1 or arg2 that is temp.
    unordered_set<string> used;
    for (auto &t : code) {
        if (t.isLabel) continue;
        if (!t.arg1.empty() && isTemp(t.arg1)) used.insert(t.arg1);
        if (!t.arg2.empty() && isTemp(t.arg2)) used.insert(t.arg2);
        // also if op is "print" and arg1 is temp, it's used (already handled)
    }

    // Now iterate backwards and mark temps that are needed because they feed into used temps
    // We'll remove any instruction that defines a temp not in used, and not used by side-effect instructions (call/print etc.)
    bool anyRemoved = true;
    while (anyRemoved) {
        anyRemoved = false;
        vector<TAC> newCode;
        for (size_t i = 0; i < code.size(); ++i) {
            TAC &t = code[i];
            if (t.isLabel) { newCode.push_back(t); continue; }
            // keep instructions that have side effects or define non-temp or are used
            bool definesTemp = isTemp(t.res);
            bool keep = true;
            if (definesTemp && used.find(t.res) == used.end()) {
                // not used — can remove only if instruction has no side-effect
                if (t.op == "assign" || t.op == "+" || t.op == "-" || t.op == "*" || t.op == "/" || t.op == "%" ||
                    t.op == "<" || t.op == ">" || t.op == "<=" || t.op == ">=" || t.op == "==" || t.op == "!=" ||
                    t.op == "&&" || t.op == "||" || t.op == "!") {
                    // safe to remove
                    keep = false;
                    anyRemoved = true;
                    changed = true;
                } else {
                    // operations like call produce side-effects (maybe) — keep
                    keep = true;
                }
            }
            if (keep) newCode.push_back(t);
            else {
                // removing t: if its args are temps, they may now become unused. Remove from used if they are temps and not referenced elsewhere.
                if (!t.arg1.empty() && isTemp(t.arg1)) {
                    bool appears = false;
                    for (size_t j = 0; j < code.size(); ++j) {
                        if (j == i) continue;
                        TAC &tt = code[j];
                        if (tt.isLabel) continue;
                        if (tt.arg1 == t.arg1 || tt.arg2 == t.arg1) { appears = true; break; }
                    }
                    if (!appears) used.erase(t.arg1);
                }
                if (!t.arg2.empty() && isTemp(t.arg2)) {
                    bool appears = false;
                    for (size_t j = 0; j < code.size(); ++j) {
                        if (j == i) continue;
                        TAC &tt = code[j];
                        if (tt.isLabel) continue;
                        if (tt.arg1 == t.arg2 || tt.arg2 == t.arg2) { appears = true; break; }
                    }
                    if (!appears) used.erase(t.arg2);
                }
            }
        }
        code.swap(newCode);
    }

    return changed;
}
