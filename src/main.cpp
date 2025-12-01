#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: lexer <filename>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cout << "Cannot open file.\n";
        return 1;
    }

    std::stringstream ss;
    ss << file.rdbuf();

    Lexer lex(ss.str());
    auto tokens = lex.tokenize();

    for (auto &t : tokens) {
        std::cout << "Token: " << t.lexeme
                  << "  Type: " << (int)t.type
                  << "  Line: " << t.line << "\n";
    }
}
