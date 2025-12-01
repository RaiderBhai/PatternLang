#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "ast_printer.h"

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

    // ===== LEXING =====
    Lexer lex(ss.str());
    auto tokens = lex.tokenize();

    // ===== PARSING =====
    Parser parser(tokens);
    Program* program = parser.parse();

    std::cout << "=== Parsing Completed Successfully ===\n\n";

    // ===== AST PRINTING =====
    ASTPrinter printer;
    printer.print(program);

    return 0;
}
