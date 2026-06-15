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

### 🏛️ The Dual-Engine Architecture (V1 vs V2)
This project contains two completely distinct execution architectures:

1. **V1 (Primary Engine - AST Evaluator)**: This is the heavily tested, highly robust main engine of this repository. It recursively evaluates the AST in real-time. It is built for 100% specification accuracy, deep closures, and comprehensive built-in APIs. **You should always use V1 as the primary engine.**
2. **V2 (Experimental Bytecode Engine & VM)**: The V2 engine abandons the AST evaluator and instead introduces a **Bytecode Compiler and Virtual Machine (VM)**. The AST is compiled into low-level operational bytecode which is then executed via a blazing fast dispatch loop in the VM. **The purpose of V2** is extreme performance, optimizing execution speeds by eliminating the overhead of recursively traversing tree nodes during runtime.

---

## ✨ Comprehensive Features 

### 🛠️ V1 (Primary AST Engine)
The V1 engine brings the essential mechanics of JavaScript to life. It has been extensively upgraded to solve profound language limitations:

- **Core Data Types & Structures**: 
  - Full native support for `Number`, `String`, `Boolean`, `Null`, and `Undefined`.
  - **Arrays**: Dynamic array allocation, bracket notation access (`arr[0]`), push/pop operations, dynamic `length` tracking, and high-order methods (`forEach`, `findIndex`).
  - **Objects**: Key-value pair hashing, nested objects, dot notation (`obj.key`), bracket notation (`obj["key"]`) resolution, and static methods (`Object.keys`, `Object.values`, `Object.entries`, `Object.assign`).
  - **Collections**: Native polyfill support for `Map` and `Set`.
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
  - Comprehensive `try...catch...finally` blocks with fully compatible Native JS Catch block wrapping (`e.name`, `e.message`).
  - Native custom error throwing with built-in Error prototypes (`Error`, `TypeError`, `SyntaxError`, `ReferenceError`, `RangeError`).
- **Expressions & Operators**: 
  - Arithmetic (`+`, `-`, `*`, `/`, `%`), Relational (`<`, `>`, `<=`, `>=`), and Strict/Loose Equality (`==`, `!=`, `===`, `!==`).
  - Logical chaining (`&&`, `||`, `!`), assignments (`+=`, `-=`), and Bitwise operations.
- **Asynchronous Event Loop Foundation**: 
  - Implementation of `setTimeout`, `setInterval`, `clearTimeout`, and `clearInterval` (Macrotasks).
  - Native `Promise` execution and microtask queueing via `queueMicrotask`. Also fully supports `Promise.all` asynchronously!
- **Built-In APIs**:
  - `console.log`.
  - Extended `Math` object (`random`, `floor`, `pow`, `max`, `min`, `round`, `abs`, `ceil`, `sqrt`, `sin`, `cos`, `tan`, `log`, `exp`, `PI`, `E`).
  - `Date.now()` implemented using low-level C++ chrono timestamps for millisecond accuracy.
- **Advanced Upgrades**:
  - **Destructuring Assignment**: Supports object and array destructuring (`let {a, b} = obj`, `let [x, y] = arr`), including advanced **Function Parameter Destructuring** (`function foo({a, b})`) via zero-risk AST injection logic.
  - **Garbage Collection**: Automated Mark-and-Sweep Memory Management.
  - **Event-Driven Architecture**: Native `EventEmitter` polyfills for `.on` and `.emit`.
  - **JSON**: Safe `JSON.stringify` with Circular Reference tracking/protection.
  - **Stack Traces**: ExecutionStackGuard for precise stack logs.
  - **String Prototypes**: Native C++ bridge for `indexOf`, `substring`, `includes`, `padStart`, etc.
  - **ES6 Modern**: `...args` rest/spread, arrow functions, Temporal Dead Zones (`let`/`const`).

### ⚡ V2 (Bytecode Compiler & VM)
The V2 Experimental Engine takes a performance-first approach, dropping recursive AST execution in favor of a Stack-Based Virtual Machine:

- **Bytecode Compiler**: Walks the Abstract Syntax Tree exactly once and translates nodes into a dense sequence of Operation Codes (OpCodes).
- **Virtual Machine (VM)**: A highly optimized, linear loop-based execution runtime.
- **Stack-Based Architecture**: Eliminates heavy recursive function calls. Operands and results are pushed/popped from an extremely fast linear stack structure (Push, Pop, Add, Sub, Call, LoadVar, StoreVar).
- **Extreme Performance**: Outperforms the AST engine in loop-heavy and CPU-intensive operations by removing memory-heavy tree traversals during execution.
- **Lexical Environment Storage**: Flattens scopes and registers variables linearly, making scope resolutions O(1) in many cases compared to deep AST hash map lookups.

---

## 🖥️ Getting Started (How to Run)

Because JSEngine is written in raw C++17, it is completely cross-platform and natively runs on any major Operating System.

### 🪟 Windows (Using MSYS2 / MinGW-w64)
1. Install [MSYS2](https://www.msys2.org/).
2. Open the MSYS2 terminal and install the GCC compiler:
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc
   ```
3. Compile and Run the Engines:
   ```bash
   # Add the compiler to your PATH
   export PATH="/c/msys64/ucrt64/bin:$PATH"
   
   # ========================================
   # Run V1 (Primary AST Engine) - RECOMMENDED
   # ========================================
   g++ -std=c++17 src/*.cpp -o JSEngine.exe
   ./JSEngine.exe tests/advanced_upgrades.js
   
   # ========================================
   # Run V2 (Bytecode Engine / VM)
   # ========================================
   g++ -std=c++17 compiler.cpp vm.o -o JSEngineV2.exe
   ./JSEngineV2.exe tests/advanced_upgrades.js
   ```

### 🐧 Linux (Ubuntu / Debian)
1. Install the `build-essential` package:
   ```bash
   sudo apt update
   sudo apt install build-essential
   ```
2. Compile and Run:
   ```bash
   # ========================================
   # Run V1 (Primary AST Engine) - RECOMMENDED
   # ========================================
   g++ -std=c++17 src/*.cpp -o JSEngine
   ./JSEngine tests/advanced_upgrades.js

   # ========================================
   # Run V2 (Bytecode Engine / VM)
   # ========================================
   g++ -std=c++17 compiler.cpp vm.o -o JSEngineV2
   ./JSEngineV2 tests/advanced_upgrades.js
   ```

### 🍎 macOS
1. Install Xcode Command Line Tools:
   ```bash
   xcode-select --install
   ```
2. Compile and Run:
   ```bash
   # ========================================
   # Run V1 (Primary AST Engine) - RECOMMENDED
   # ========================================
   g++ -std=c++17 src/*.cpp -o JSEngine
   ./JSEngine tests/advanced_upgrades.js

   # ========================================
   # Run V2 (Bytecode Engine / VM)
   # ========================================
   g++ -std=c++17 compiler.cpp vm.o -o JSEngineV2
   ./JSEngineV2 tests/advanced_upgrades.js
   ```

---

## 📁 Project Structure

```text
JSEngine/
├── src/
│   ├── main.cpp         # CLI Entry Point (V1)
│   ├── lexer.cpp        # Lexical Tokenizer
│   ├── parser.cpp       # AST Construction
│   ├── evaluator.cpp    # AST Execution & Event Loop
│   ├── builtins.cpp     # Native APIs (console, Math)
│   └── gc.h             # Mark-and-Sweep Garbage Collector
├── compiler.o           # V2 Bytecode Compiler Logic
├── vm.o                 # V2 Virtual Machine Logic
├── tests/
│   ├── advanced_upgrades.js  # V1 Integration & Stress Tests
│   └── test_1.js             # V1 Core Mechanic Tests
└── README.md
```

---

<div align="center">
  <i>Made with ❤️ by Adarsh</i>
</div>
