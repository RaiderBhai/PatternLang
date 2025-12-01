#include <iostream>
#include <fstream>
#include <sstream>

#include "lexer.h"
#include "parser.h"
#include "ast_printer.h"
#include "sema.h"
#include "ir_generator.h"
#include "optimizer.h"     // <<< NEW for Day 5 Optimizer
#include "codegen.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: compiler <filename>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cout << "Cannot open file.\n";
        return 1;
    }

    std::stringstream ss;
    ss << file.rdbuf();

    // ==========================
    // 1. LEXICAL ANALYSIS
    // ==========================
    Lexer lex(ss.str());
    auto tokens = lex.tokenize();

    // ==========================
    // 2. PARSING
    // ==========================
    Parser parser(tokens);
    Program* program = parser.parse();
    std::cout << "=== Parsing Completed Successfully ===\n";

    // ==========================
    // 3. SEMANTIC ANALYSIS
    // ==========================
    SemanticAnalyzer sema;
    sema.analyze(program);
    std::cout << "=== Semantic Analysis Completed Successfully ===\n";

    // ==========================
    // 4. INTERMEDIATE CODE (TAC)
    // ==========================
    IRGenerator irgen;
    irgen.generate(program);

    std::cout << "\n=== RAW TAC (Before Optimization) ===\n";
    for (auto &t : irgen.getTAC()) {
        std::cout << t.toString() << "\n";
    }

    // ==========================
    // 5. OPTIMIZATION PASS
    // ==========================
    Optimizer opt;
    TACList optimized = opt.optimize(irgen.getTAC());

    std::cout << "=== OPTIMIZED TAC ===\n";
    for (auto &t : optimized) {
        std::cout << t.toString() << "\n";
    }
    std::cout << "=== END OPTIMIZED TAC ===\n\n";

    CodeGenerator cg;
    std::string cppSrc = cg.generate(optimized, program);

    std::ofstream fout("output.cpp");
    if (!fout) {
        std::cerr << "Cannot write output.cpp\n";
    } else {
        fout << cppSrc;
        fout.close();
        std::cout << "Generated output.cpp — compile with: g++ output.cpp -o out && ./out\n";
    }

    // ==========================
    // 6. AST PRINTING (OPTIONAL)
    // ==========================
    ASTPrinter printer;
    printer.print(program);

    return 0;
}
