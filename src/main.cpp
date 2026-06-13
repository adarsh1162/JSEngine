#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"

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

    // Initialize Lexer
    Lexer lexer(sourceCode);
    std::vector<Token> tokens = lexer.tokenize();

    // Parse into AST
    Parser parser(tokens);
    std::shared_ptr<Program> ast;
    try {
        ast = parser.parse();
    } catch (const std::exception& e) {
        std::cerr << "Parser error: " << e.what() << std::endl;
        return 1;
    }

    // Evaluate AST
    Evaluator evaluator;
    evaluator.interpret(ast);

    return 0;
}
