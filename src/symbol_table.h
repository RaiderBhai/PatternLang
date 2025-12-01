#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>

enum class TypeKind {
    TYPE_INT,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_VOID,   // for functions returning nothing (if you allow)
    TYPE_UNKNOWN
};

struct Type {
    TypeKind kind;
    Type(TypeKind k = TypeKind::TYPE_UNKNOWN) : kind(k) {}
    std::string toString() const {
        switch(kind) {
            case TypeKind::TYPE_INT: return "int";
            case TypeKind::TYPE_BOOL: return "bool";
            case TypeKind::TYPE_STRING: return "string";
            case TypeKind::TYPE_VOID: return "void";
            default: return "unknown";
        }
    }
    bool operator==(const Type &o) const { return kind == o.kind; }
    bool operator!=(const Type &o) const { return kind != o.kind; }
};

struct Symbol {
    std::string name;
    Type type;
    bool isFunction = false;
    // function signature
    std::vector<Type> paramTypes;
    Type returnType = Type(TypeKind::TYPE_UNKNOWN);

    Symbol() = default;
    Symbol(std::string n, Type t) : name(n), type(t) {}
};

// A simple scope-based symbol table
class SymbolTable {
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
public:
    SymbolTable() { pushScope(); } // global scope

    void pushScope() { scopes.emplace_back(); }
    void popScope() { if (!scopes.empty()) scopes.pop_back(); }

    bool insert(const Symbol &s) {
        if (scopes.empty()) pushScope();
        auto &cur = scopes.back();
        if (cur.find(s.name) != cur.end()) return false;
        cur[s.name] = s;
        return true;
    }

    // find in current scope only
    bool existsInCurrent(const std::string &name) {
        if (scopes.empty()) return false;
        auto &cur = scopes.back();
        return cur.find(name) != cur.end();
    }

    // lookup through all scopes (inner -> outer)
    Symbol* lookup(const std::string &name) {
        for (int i = (int)scopes.size() - 1; i >= 0; --i) {
            auto it = scopes[i].find(name);
            if (it != scopes[i].end()) return &it->second;
        }
        return nullptr;
    }

    // convenience for top-level (global) insertion
    bool insertGlobal(const Symbol &s) {
        if (scopes.empty()) pushScope();
        auto &g = scopes.front();
        if (g.find(s.name) != g.end()) return false;
        g[s.name] = s;
        return true;
    }
};

#endif

