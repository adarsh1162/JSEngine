# Technologies Used

This document outlines the core technologies and standard libraries used to build the C++ JavaScript execution engine from scratch.

## Core Language & Tooling
- **C++17 or C++20**: Utilizing modern C++ features (like `std::variant`, `std::shared_ptr`, `std::unique_ptr`, `std::unordered_map`) to safely and efficiently manage dynamic types and memory, which is essential for a JavaScript runtime.
- **CMake**: The build system to ensure cross-platform compatibility, easy compilation, and industry-standard project structure.
- **GCC / Clang / MSVC**: The C++ compilers used to build the final executable.

## Architecture Patterns
- **Lexical Analysis (Lexer)**: Built from scratch to convert plain text into streams of manageable Tokens.
- **Recursive Descent Parsing**: A top-down parsing technique used to build the Abstract Syntax Tree (AST) without relying on external parser generators like Bison or Yacc.
- **Tree-Walking Interpreter**: For execution, the AST is traversed recursively, evaluating expressions and statements within isolated execution scopes.

## C++ Standard Library Components
- `std::string` and `std::string_view`: For efficient string parsing and manipulation (representing JS strings).
- `std::vector`: The backbone for the Lexer's token stream, AST child nodes, and the internal representation of JavaScript Arrays.
- `std::unordered_map`: Used to implement JavaScript Objects (key-value pairs) and global/local Environment Scopes for variable resolution.
- `std::variant` & `std::any`: To handle the dynamic typing nature of JavaScript (a variable can hold a number, string, object, or function at runtime).
- `std::shared_ptr` / `std::weak_ptr`: For automatic memory management (garbage collection simulation) of JavaScript objects, arrays, and closures, preventing memory leaks without writing a custom Garbage Collector.

## Quality & Verification Tools
- **Valgrind / AddressSanitizer (ASan)**: (Optional but recommended) to ensure the engine has no memory leaks or undefined behaviors.
- **Google Test (gtest) / Catch2**: (Optional) For internal C++ unit testing, ensuring the Lexer and Parser components function perfectly before full evaluation.
