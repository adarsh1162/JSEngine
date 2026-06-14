<div align="center">
  <h1>🚀 JSEngine</h1>
  <h3>A highly robust, high-performance JavaScript engine built entirely from scratch in C++17.</h3>
  <p>JSEngine is a deeply embedded language runtime designed to parse, interpret, and execute JavaScript. It features a custom lexical tokenizer, a recursive descent AST parser, and a fully functional evaluation engine equipped with an asynchronous Event Loop, Deep Closures, and Mark-and-Sweep Garbage Collection.</p>
</div>

---

## 🌟 Introduction

Welcome to **JSEngine**! This project is an ambitious endeavor to reconstruct the core mechanics of a JavaScript runtime environment from the ground up, with **zero external dependencies** (no V8, SpiderMonkey, or QuickJS). 

Instead of relying on pre-existing interpreters, JSEngine implements every phase of code execution natively:
- **Lexical Analysis**: Converts raw string inputs into meaningful streams of tokens.
- **Syntactic Analysis (Parser)**: Constructs a highly detailed Abstract Syntax Tree (AST) representing complex logic.
- **Evaluation Engine**: Executes the AST while seamlessly managing memory, dynamic scopes, contexts, and closures.
- **Event Loop**: Accurately mimics the NodeJS non-blocking concurrency model via separate Macro and Microtask queues.

---

## ✨ Comprehensive Features

### 🛠️ V1 (Core Engine Architecture)
The initial architecture of JSEngine brings the essential mechanics of JavaScript to life:

- **Core Data Types & Structures**: 
  - Full native support for `Number`, `String`, `Boolean`, `Null`, and `Undefined`.
  - **Arrays**: Dynamic array allocation, bracket notation access (`arr[0]`), push/pop operations, and dynamic `length` tracking.
  - **Objects**: Key-value pair hashing, nested objects, dot notation (`obj.key`), and bracket notation (`obj["key"]`) resolution.
- **Functions & Execution Contexts**: 
  - First-class citizens: Function declarations, anonymous functions, and function expressions.
  - **Deep Closures**: Functions accurately capture and retain their lexical environment scopes.
  - Context binding: Native `this` keyword resolution and automatic `arguments` object injection.
- **Control Flow Mechanisms**: 
  - Deep implementations of `if/else`, `while`, and `for` loops.
  - Control jumps: Nested `break`, `continue`, and `return` logic.
- **Object-Oriented Programming**: 
  - Complete ES6 `class` syntax support.
  - Constructors, methods, prototype chain delegation, inheritance (`extends`), and `super` property calls.
- **Error Handling**: 
  - Comprehensive `try...catch...finally` blocks.
  - Native custom error throwing (`throw new Error()`).
- **Expressions & Operators**: 
  - Arithmetic (`+`, `-`, `*`, `/`, `%`), Relational (`<`, `>`, `<=`, `>=`), and Strict/Loose Equality (`==`, `!=`, `===`, `!==`).
  - Logical chaining (`&&`, `||`, `!`), assignments (`+=`, `-=`), and Bitwise operations.
- **Asynchronous Event Loop Foundation**: 
  - Implementation of `setTimeout`, `setInterval`, `clearTimeout`, and `clearInterval` (Macrotasks).
  - Promise microtask execution via `queueMicrotask`.
- **Built-In APIs**:
  - `console.log`, `Math` object methods (`random`, `floor`, `abs`, etc).

### 🚀 V2 (Advanced System Upgrades)
The V2 update massively bridges the gap between a basic interpreter and a production-ready JavaScript runtime by solving profound language limitations:

- **Advanced Memory Management (Garbage Collection)**: 
  - Implements an automated, asynchronous **Mark-and-Sweep Garbage Collector**.
  - Periodically triggers off the Event Loop to sweep unreachable objects, closures, and environments, completely eliminating memory leaks during intense workloads.
- **Native Event-Driven Architecture**: 
  - A fully integrated, NodeJS-style `EventEmitter` module natively polyfilled into the engine.
  - Seamlessly handles `.on()` listeners and `.emit()` dispatches asynchronously without blocking the main execution thread.
- **JSON Serialization & Protection**: 
  - Deep `JSON.stringify` and `JSON.parse` implementations.
  - **Circular Reference Protection**: A critical safeguard that automatically tracks visited pointers via `std::unordered_set` to safely throw `TypeError` instead of crashing via infinite recursion/stack overflow when serializing self-referencing objects.
- **Detailed Error Stack Traces**: 
  - An intelligent execution stack tracker (`ExecutionStackGuard`) logs the precise depth and hierarchy of function calls, rendering highly accurate runtime Stack Traces when an error is thrown.
- **Native String & Array Prototype Extension**: 
  - Perfected prototype chain fallback delegation bridging native C++ evaluation and JS.
  - Comprehensive String API methods natively injected: `indexOf`, `substring`, `includes`, `startsWith`, `endsWith`, `concat`, `padStart`, `padEnd`.
- **Modern ES6 Integration**: 
  - **Rest & Spread Syntax**: Unpacking iterables (`...args`, `[...arr]`) perfectly handled via AST spread element parsing.
  - **Arrow Functions**: `() => {}` with accurate `this` lexical binding overrides.
  - **Temporal Dead Zone (TDZ)**: Lexical bindings for `let` and `const` variables ensuring strict block-scoped temporal dead zones.
  - Deep Recursion Stack Limit protection seamlessly incorporated to prevent OS-level segmentation faults.

---

## 🖥️ Getting Started (How to Run)

Because JSEngine is written in raw C++17, it is completely cross-platform and natively runs on any major Operating System.

### 🪟 Windows (Using MSYS2 / MinGW-w64)
1. Install [MSYS2](https://www.msys2.org/).
2. Open the MSYS2 terminal and install the GCC compiler:
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc
   ```
3. Compile and Run:
   ```bash
   # Add the compiler to your PATH
   export PATH="/c/msys64/ucrt64/bin:$PATH"
   
   # Compile the Engine
   g++ -std=c++17 src/*.cpp -o JSEngine.exe
   
   # Run a JS file
   ./JSEngine.exe tests/advanced_upgrades.js
   ```

### 🐧 Linux (Ubuntu / Debian)
1. Install the `build-essential` package:
   ```bash
   sudo apt update
   sudo apt install build-essential
   ```
2. Compile and Run:
   ```bash
   g++ -std=c++17 src/*.cpp -o JSEngine
   ./JSEngine tests/advanced_upgrades.js
   ```

### 🍎 macOS
1. Install Xcode Command Line Tools:
   ```bash
   xcode-select --install
   ```
2. Compile and Run:
   ```bash
   g++ -std=c++17 src/*.cpp -o JSEngine
   ./JSEngine tests/advanced_upgrades.js
   ```

---

## 📁 Project Structure

```text
JSEngine/
├── src/
│   ├── main.cpp         # CLI Entry Point
│   ├── lexer.cpp        # Lexical Tokenizer
│   ├── parser.cpp       # AST Construction
│   ├── evaluator.cpp    # AST Execution & Event Loop
│   ├── builtins.cpp     # Native APIs (console, Math)
│   └── gc.h             # Mark-and-Sweep Garbage Collector
├── tests/
│   ├── advanced_upgrades.js  # V2 Integration & Stress Tests
│   └── test_1.js             # V1 Core Mechanic Tests
└── README.md
```

---

<div align="center">
  <i>Built with ❤️ for JavaScript and C++ by passionate engineers.</i>
</div>
