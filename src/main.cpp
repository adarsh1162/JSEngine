#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstring>
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"
#include "vm.h"
#include "compiler.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef CONST
#undef IN
#undef OUT
#undef ERROR
#endif

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
    // Check for Embedded Payload First!
    std::string exePath = argv[0];
#ifdef _WIN32
    char pathBuf[MAX_PATH];
    if (GetModuleFileNameA(NULL, pathBuf, MAX_PATH)) {
        exePath = pathBuf;
    }
#endif

    std::ifstream selfFile(exePath, std::ios::binary | std::ios::ate);
    if (selfFile.is_open()) {
        std::streamsize size = selfFile.tellg();
        if (size > 8) {
            selfFile.seekg(-8, std::ios::end);
            uint32_t length = 0;
            uint32_t magic = 0;
            selfFile.read(reinterpret_cast<char*>(&length), 4);
            selfFile.read(reinterpret_cast<char*>(&magic), 4);
            
            if (magic == 0x5845534A) { // 'JSEX'
                if (size >= 8 + static_cast<std::streamsize>(length)) {
                    selfFile.seekg(-8 - static_cast<std::streamoff>(length), std::ios::end);
                    std::string payload(length, '\0');
                    selfFile.read(&payload[0], length);
                    run(payload);
                    return 0; // Terminate after running embedded payload
                }
            }
        }
    }

    std::string sourceCode;
    std::string filePath;
    bool compileMode = false;
    std::string compileOutput = "app.exe";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--vm") {
            useVM = true;
        } else if (arg == "--compile") {
            compileMode = true;
            if (i + 1 < argc && std::string(argv[i+1]) != "-o") {
                filePath = argv[++i];
            }
        } else if (arg == "-o") {
            if (i + 1 < argc) compileOutput = argv[++i];
        } else if (filePath.empty()) {
            filePath = arg;
        }
    }

    if (compileMode) {
        if (filePath.empty()) {
            std::cerr << "Error: No input file specified for --compile.\n";
            return 1;
        }
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filePath << std::endl;
            return 1;
        }
        std::string payload((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        std::ifstream srcExe(exePath, std::ios::binary);
        std::ofstream destExe(compileOutput, std::ios::binary);
        if (!srcExe.is_open() || !destExe.is_open()) {
            std::cerr << "Error: Could not create output binary.\n";
            return 1;
        }
        
        // Copy engine binary
        destExe << srcExe.rdbuf();
        
        // Append JS Payload
        destExe.write(payload.data(), payload.size());
        
        // Append Footer
        uint32_t length = payload.size();
        uint32_t magic = 0x5845534A; // JSEX
        destExe.write(reinterpret_cast<char*>(&length), 4);
        destExe.write(reinterpret_cast<char*>(&magic), 4);
        
        std::cout << "Successfully compiled '" << filePath << "' to '" << compileOutput << "'\n";
        return 0;
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
