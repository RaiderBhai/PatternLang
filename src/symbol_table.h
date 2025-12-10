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
    TYPE_VOID,
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
    bool isBuiltin = false;
    std::vector<Type> paramTypes;
    Type returnType = Type(TypeKind::TYPE_UNKNOWN);

    Symbol() = default;
    Symbol(std::string n, bool isFunc, std::vector<Type> params, Type retType)
        : name(n), isFunction(isFunc), paramTypes(params), returnType(retType) {}
};

class SymbolTable {
private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
    int scopeCounter = 0;
    // Store snapshots of scopes as they are created (for hierarchical printing)
    std::vector<std::unordered_map<std::string, Symbol>> scopeSnapshots;
    bool captureScopes = true; // Flag to control whether to capture scopes

public:
    SymbolTable() { pushScope(); }

    void pushScope() {
        scopes.emplace_back();
    }

    void popScope(bool capture = true) {
        if (!scopes.empty()) {
            // Save a snapshot before popping (only if capture is true)
            if (capture) {
                scopeSnapshots.push_back(scopes.back());
            }
            scopes.pop_back();
        }
    }

    bool insert(const Symbol &s) {
        auto &cur = scopes.back();
        if (cur.find(s.name) != cur.end())
            return false;
        cur[s.name] = s;
        return true;
    }

    bool existsInCurrent(const std::string &name) {
        return scopes.back().find(name) != scopes.back().end();
    }

    Symbol* lookup(const std::string &name) {
        for (int i = scopes.size() - 1; i >= 0; i--) {
            auto it = scopes[i].find(name);
            if (it != scopes[i].end())
                return &it->second;
        }
        return nullptr;
    }

    bool insertGlobal(const Symbol &s) {
        auto &g = scopes.front();
        if (g.find(s.name) != g.end())
            return false;
        g[s.name] = s;
        return true;
    }

    void print() {
        std::cout << "\n=== SYMBOL TABLE ===\n";

        // Print Scope 0 (global) - exclude built-in functions
        std::cout << "Scope 0:\n";
        std::cout << "  Name                Kind        Type                Params -> Return\n";
        std::cout << "  ------------------------------------------------------------------------\n";
        
        if (!scopes.empty()) {
            for (auto &p : scopes[0]) {
                const Symbol &sym = p.second;
                // Skip built-in functions in the main scope output
                if (sym.isBuiltin) continue;

                std::cout << "  " << sym.name;
                int pad = 20 - sym.name.size();
                if (pad < 1) pad = 1;
                std::cout << std::string(pad, ' ');

                if (sym.isFunction) {
                    std::cout << "function    " << sym.returnType.toString();
                    
                    // Pad the type column
                    int typePad = 20 - sym.returnType.toString().size();
                    if (typePad < 1) typePad = 1;
                    std::cout << std::string(typePad, ' ');
                    
                    std::cout << "(";
                    for (size_t i = 0; i < sym.paramTypes.size(); i++) {
                        std::cout << sym.paramTypes[i].toString();
                        if (i + 1 < sym.paramTypes.size()) std::cout << ", ";
                    }
                    std::cout << ") -> " << sym.returnType.toString();
                } else {
                    std::cout << "variable    " << sym.type.toString();
                    int typePad = 20 - sym.type.toString().size();
                    if (typePad < 1) typePad = 1;
                    std::cout << std::string(typePad, ' ');
                    std::cout << "-";
                }

                std::cout << "\n";
            }
        }
        std::cout << "\n";

        // Print all captured scope snapshots as Scope 1, 2, 3, etc.
        int scopeId = 1;
        for (auto &scope : scopeSnapshots) {
            // Skip empty scopes
            if (scope.empty()) continue;
            
            std::cout << "Scope " << scopeId++ << ":\n";
            std::cout << "  Name                Kind        Type                Params -> Return\n";
            std::cout << "  ------------------------------------------------------------------------\n";
            
            for (auto &p : scope) {
                const Symbol &sym = p.second;
                
                std::cout << "  " << sym.name;
                int pad = 20 - sym.name.size();
                if (pad < 1) pad = 1;
                std::cout << std::string(pad, ' ');

                if (sym.isFunction) {
                    std::cout << "function    " << sym.returnType.toString();
                    int typePad = 20 - sym.returnType.toString().size();
                    if (typePad < 1) typePad = 1;
                    std::cout << std::string(typePad, ' ');
                    std::cout << "(";
                    for (size_t i = 0; i < sym.paramTypes.size(); i++) {
                        std::cout << sym.paramTypes[i].toString();
                        if (i + 1 < sym.paramTypes.size()) std::cout << ", ";
                    }
                    std::cout << ") -> " << sym.returnType.toString();
                } else {
                    std::cout << "variable    " << sym.type.toString();
                    int typePad = 20 - sym.type.toString().size();
                    if (typePad < 1) typePad = 1;
                    std::cout << std::string(typePad, ' ');
                    std::cout << "-";
                }

                std::cout << "\n";
            }
            std::cout << "\n";
        }

        std::cout << "=== END SYMBOL TABLE ===\n\n";
    }
};

#endif