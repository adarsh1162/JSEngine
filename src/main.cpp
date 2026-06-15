#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"
#include "vm.h"
#include "compiler.h"

VM vm; // Global VM instance for V2 compatibility
bool useVM = false;

void run(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    std::shared_ptr<Program> program = nullptr;
    try {
        Parser parser(tokens);
        program = parser.parse();
    } catch(const std::exception& e) {
        std::cerr << "Syntax Error: " << e.what() << "\n";
        return;
    }

    if (!program) {
        std::cerr << "Syntax Error: Parsing failed.\n";
        return;
    }

    if (useVM) {
        ObjInstance* consoleObj = allocateInstance(nullptr);
        ObjNative* logNative = allocateNative([](int argCount, Value* args) {
            for (int i = 0; i < argCount; i++) {
                printValue(args[i]);
                if (i < argCount - 1) std::cout << " ";
            }
            std::cout << std::endl;
            return UNDEFINED_VAL();
        }, "log");
        consoleObj->fields["log"] = OBJ_VAL(logNative);
        ObjNative* queueNative = allocateNative([](int argCount, Value* args) {
            if (argCount >= 1) {
                vm.enqueueMicrotask(args[0], UNDEFINED_VAL());
            }
            return UNDEFINED_VAL();
        }, "queueMicrotask");

        vm.globals["console"] = OBJ_VAL(consoleObj);
        vm.globals["queueMicrotask"] = OBJ_VAL(queueNative);

        AstCompiler compiler;
        ObjFunction* function = compiler.compile(program);
        if (function) {
            InterpretResult result = vm.interpret(function);
        } else {
            std::cerr << "Compile Error: Bytecode generation failed.\n";
        }
    } else {
        try {
            Evaluator evaluator;
            evaluator.interpret(program);
        } catch (const std::exception& e) {
            std::cerr << "Runtime Error: " << e.what() << "\n";
        } catch (...) {
            std::cerr << "Unknown Runtime Error!\n";
        }
    }
}

void runRepl() {
    std::cout << "Welcome to JSEngine V1 REPL!\nType '.exit' to quit.\n";
    if (useVM) {
        std::cout << "Note: REPL does not support V2 VM yet. Running in V1 AST Mode.\n";
    }
    
    Evaluator evaluator;
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line == ".exit") break;
        if (line.empty()) continue;

        try {
            Lexer lexer(line);
            auto tokens = lexer.tokenize();
            Parser parser(tokens);
            auto program = parser.parse();
            if (!program) {
                std::cerr << "Syntax Error.\n";
                continue;
            }
            evaluator.interpret(program);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        } catch (...) {
            std::cerr << "Unknown Error\n";
        }
    }
}

int main(int argc, char* argv[]) {
    std::string sourceCode;
    std::string filePath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--vm") {
            useVM = true;
        } else {
            filePath = arg;
        }
    }

    if (!filePath.empty()) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filePath << std::endl;
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        sourceCode = buffer.str();
        run(sourceCode);
    } else {
        runRepl();
    }

    return 0;
}
