# JavaScript Interpreter/Compiler in C++ - Implementation Plan

## Goal Description
The objective is to build an end-to-end JavaScript execution engine (Interpreter/Compiler) written in C++ **entirely from scratch**. The program will read JavaScript code (via file, stdin, or CLI arguments), parse it, execute it, and output the correct result to `stdout`. 

**Critical Requirement:** The engine will NOT be hardcoded to pass only the 5 test cases. It will be robust enough to handle **any valid JavaScript code** containing the supported features. We are participating in a hackathon, so innovation, zero external dependencies (no V8, no QuickJS), and full scratch implementation are mandatory.

## UI vs. Command Line (CLI) Decision
**Recommendation: Direct Code (CLI) is the right approach.**
Building a UI for a compiler/interpreter project introduces unnecessary overhead and distracts from the core goal: **Performance, Code Quality, and Execution Accuracy.** Compilers and runtimes (like Node.js, V8, GCC) are inherently command-line tools. By focusing strictly on a CLI, we can ensure the architecture remains pure, lightweight, and strictly focused on parsing and executing JavaScript at blazing speeds. 

## Implementation Approach
**Architecture:** Option A (AST Tree-Walking Interpreter)
Since we must build everything from scratch for a hackathon, an **AST Tree-Walking Interpreter** is the best choice. It allows us to implement complex features (closures, callbacks, spread/rest operators) robustly and maintain an industry-level codebase within a realistic timeframe. 

**Built-in Objects (Math, Date) and Methods:**
*All* specified Array, String, Math, and Date methods will be implemented natively from scratch in C++. 

## Proposed Architecture

The C++ JS Engine will be divided into the following core components:

1. **Lexer (Tokenizer):** Reads the raw JS string and converts it into a stream of tokens (Keywords, Identifiers, Operators, Literals).
2. **Parser:** Takes tokens and builds an Abstract Syntax Tree (AST) using Recursive Descent Parsing.
3. **Environment (Scope Manager):** Handles variable scoping (let/const), closures, and memory mapping.
4. **Runtime/Evaluator:** Traverses the AST and executes the logic.
5. **Standard Library (JS Native Code in C++):** Pure C++ implementations of `console.log`, `Array`, `String`, `Math`, `Date`, etc.

## Milestones

### Milestone 1: Core Foundation & Lexer
- Setup C++ project structure (CMake).
- Implement the `Token` and `Lexer` classes from scratch.
- Support lexing for numbers, strings, identifiers, operators, and keywords.

### Milestone 2: Parser & AST (Abstract Syntax Tree)
- Define AST node classes (Expressions, Statements, Binary Operations, Literals).
- Implement a Recursive Descent Parser from scratch.
- Handle Variable Declarations (`let`, `const`) and basic Arithmetic/Logical operators.

### Milestone 3: Basic Evaluator & Scope
- Implement the `Environment` class for lexical scoping.
- Implement the `Evaluator` to walk the AST and compute basic expressions.
- Implement `console.log` natively to pass Test Case 1 (Odd/Even Checker).
- Handle Conditional statements (`if`, `else if`, `switch`).

### Milestone 4: Control Flow & Functions
- Implement Loops (`for`, `while`, `do...while`) to pass Test Case 2 (Triangle Pattern).
- Implement Function declarations, Arrow functions, Callbacks, and Return statements.
- Implement closures and execution contexts to pass Test Case 3 (Armstrong Number).

### Milestone 5: Arrays, Strings, and Built-ins
- Implement JS `Array` wrapper in C++ (with methods like `push`, `pop`, `reverse`, `join`, `map`, `filter`, etc.) entirely from scratch.
- Pass Test Case 4 (Array Reverse).
- Implement JS `String` wrapper (with methods like `split`, `replace`, `substring`, `trim`, etc.).
- Pass Test Case 5 (String Palindrome Check).
- Implement `Math` and `Date` native objects.

### Milestone 6: Advanced Features & Memory
- Implement Objects (Dictionaries/Hashmaps in C++).
- Implement Spread (`...`) and Rest operators.
- Type conversion and coercion logic.
- Code cleanup, applying strictly humanized and necessary comments.

## Verification Plan
- Run all 5 provided JS test cases through the compiled C++ executable.
- Run hidden test cases (complex valid JS code) to ensure robustness.
- Check memory leaks to ensure industry-level code quality.