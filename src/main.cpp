#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"
#include "vm.h"

VM vm; // Global VM instance for V2 compatibility

void run(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto program = parser.parse();

    if (!program) {
        std::cerr << "Syntax Error: Parsing failed.\n";
        return;
    }

    Evaluator evaluator;
    try {
        evaluator.interpret(program);
    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << "\n";
    }
}

int main(int argc, char* argv[]) {
    std::string sourceCode;

    if (argc > 1) {
        // Read from file
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << argv[1] << std::endl;
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        sourceCode = buffer.str();
    } else {
        // Read from stdin (for basic testing without file)
        std::cout << "Enter JavaScript code (Press Ctrl+D to finish on Linux/Mac, Ctrl+Z on Windows):" << std::endl;
        std::stringstream buffer;
        buffer << std::cin.rdbuf();
        sourceCode = buffer.str();
    }

    run(sourceCode);

    return 0;
}
