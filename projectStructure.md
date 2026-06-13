# Project Structure

This document outlines the directory and file structure for the C++ JavaScript Engine. Everything is organized cleanly to maintain industry-level code quality and readability.

```text
JavaScript/
├── src/
│   ├── main.cpp          // CLI Entry Point: Handles file reading and execution
│   ├── lexer.h           // Tokenizer definition
│   ├── lexer.cpp         // Tokenizer implementation
│   ├── parser.h          // AST Parser definition
│   ├── parser.cpp        // AST Parser implementation
│   ├── ast.h             // Abstract Syntax Tree Nodes
│   ├── environment.h     // Scope & Variable management (Closures)
│   ├── environment.cpp   // Environment implementation
│   ├── evaluator.h       // AST execution logic (Tree Walker)
│   ├── evaluator.cpp     // Evaluator implementation
│   ├── types.h           // JS Value Representations (Number, String, Object, Array, Function)
│   ├── types.cpp         // JS Types implementation
│   ├── builtins.h        // Standard Library (console, Math, Date, Array methods)
│   └── builtins.cpp      // Builtins implementation
├── tests/
│   ├── test1.js          // Odd or Even Checker
│   ├── test2.js          // Triangle Pattern
│   ├── test3.js          // Armstrong Number
│   ├── test4.js          // Array Reverse
│   └── test5.js          // Palindrome Check
├── CMakeLists.txt        // Build configuration for the C++ project
├── implementation.md     // Project roadmap and plan
├── projectStructure.md   // This file
├── projectDetail.md      // User requirements
├── ProjectQuality.md     // Quality standards
├── tech.md               // Technologies used
└── testCases.md          // Provided test cases
```

## Component Breakdown

1. **Lexer (`lexer.h`, `lexer.cpp`)**: Scans the input JavaScript code character by character and groups them into meaningful `Token`s.
2. **Parser (`parser.h`, `parser.cpp`, `ast.h`)**: Takes the stream of tokens and builds an Abstract Syntax Tree (AST), ensuring the syntax is valid.
3. **Environment (`environment.h`, `environment.cpp`)**: Manages the scoping rules (block scope for `let`/`const`) and stores variable values.
4. **Types (`types.h`, `types.cpp`)**: Represents dynamic JavaScript variables in C++ using advanced techniques (like `std::variant` or base pointer classes) so a variable can be a string, number, or function.
5. **Evaluator (`evaluator.h`, `evaluator.cpp`)**: Walks through the AST, computes the logic, accesses the Environment, and produces the result.
6. **Builtins (`builtins.h`, `builtins.cpp`)**: The C++ code that implements standard JavaScript features like `console.log`, `Math.random()`, and `Array.prototype.map()`.
