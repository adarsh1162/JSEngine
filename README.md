<div align="center">

# C++ JavaScript Engine
**A high-performance, handcrafted JavaScript interpreter written entirely from scratch in C++17.**

</div>

## 🚀 Introduction

Welcome to the **C++ JavaScript Engine**! This project is a fully functional, lightweight, and extensible JavaScript interpreter built from the ground up without relying on heavy external dependencies like V8 or SpiderMonkey. 

Designed with a focus on deep ECMAScript specification compliance, this engine features its own Lexer, Recursive Descent Parser, AST Evaluator, and a custom **Mark-and-Sweep Garbage Collector**. It successfully bridges the gap between C++ performance and JavaScript's dynamic flexibility, fully simulating the JavaScript Event Loop, asynchronous Microtasks/Macrotasks, and complex prototype chains natively.

Whether you're looking to embed a JS engine into a C++ game/application, learn about compiler design, or just experiment with a highly optimized custom JS runtime, this engine provides a solid, modern foundation.

---

## 🛠️ Build & Run Instructions

This engine is written in standard `C++17` and has **zero external dependencies** other than the C++ Standard Template Library (STL). You can easily compile and run it on any major operating system.

### 🪟 Windows (via MSYS2 / MinGW)
```bash
# Compile the engine
g++ -std=c++17 src/*.cpp -o JSEngine.exe

# Run a JavaScript file
./JSEngine.exe tests/test.js
```

### 🐧 Linux (via GCC or Clang)
```bash
# Compile the engine
g++ -std=c++17 src/*.cpp -o JSEngine

# Run a JavaScript file
./JSEngine tests/test.js
```

### 🍎 macOS (via Apple Clang)
```bash
# Compile the engine
clang++ -std=c++17 src/*.cpp -o JSEngine

# Run a JavaScript file
./JSEngine tests/test.js
```

---

## 🌟 Implemented Features

This engine has undergone rigorous testing (including an Ultimate 13-Limitation Test Suite) and successfully supports the following modern ECMAScript features:

### 1. Core Language & Syntax
* **Variables & Scoping:** Complete support for `let`, `const`, and `var`. Fully enforces block-scoping and **Temporal Dead Zone (TDZ)** to trap premature variable access.
* **Data Types:** Numbers, Strings, Booleans, Null, Undefined, Arrays, Objects, and Functions.
* **Operators:** Arithmetic, Logical (`&&`, `||`, `!`), Bitwise, Comparison (Strict `===` and Loose `==` equality), Ternary (`? :`), and Spread operators (`...`).
* **Control Flow:** `if/else`, `switch/case`, `while`, `do-while`, `for`, `for...of`, `break`, and `continue`.
* **Error Handling:** Full `try...catch...finally` and `throw` statement support with native stack trace bridging.
* **Modern Syntax:** Template literals (with string interpolation), Object and Array destructuring, and Default function parameters.

### 2. Object-Oriented Programming (OOP)
* **Classes:** ES6 `class` syntax, constructors, and static methods.
* **Inheritance:** `extends` and `super` keyword support for complex class hierarchies.
* **Prototypes:** Deep integration of the global `Object.prototype` chain (e.g., native fallback for `hasOwnProperty` and `toString`).
* **Properties:** Support for `Object.defineProperty`, complete with natively intercepted **Getters and Setters**.
* **Context Binding:** Strict execution context bridging for the `this` keyword and implicit `arguments` object injection for non-arrow functions.

### 3. Built-in Objects & Methods
* **Array:** `push`, `pop`, `shift`, `unshift`, `map`, `filter`, `reduce`, `find`, `some`, `every`, `indexOf`, and bracket notation get/set.
* **String:** `toLowerCase`, `substring`, etc.
* **Math:** `Math.random()`, `Math.floor()`, etc.
* **Date:** `Date.now()`, `getTime()`.
* **JSON:** Native bindings for `JSON.parse` and `JSON.stringify` (powered by the engine's internal AST parser).
* **Collections:** Fully compliant `Map` and `Set` implementations (bridged natively for performance).
* **Console:** `console.log`, `console.error`, `console.time`, `console.timeEnd`.

### 4. Asynchronous Execution & Event Loop
* **Event Loop Engine:** Accurate simulation of the JavaScript event loop queue inside C++.
* **Macrotasks:** `setTimeout` scheduling.
* **Microtasks:** Native `queueMicrotask` queueing mechanism.
* **Promises:** A fully working `Promise` constructor with asynchronous chained `.then()`, `.catch()`, `Promise.resolve()`, and `Promise.reject()`. Priority execution order between microtasks and macrotasks is completely enforced.

### 5. Memory Management & Safety
* **Mark-and-Sweep Garbage Collector:** Custom GC that tracks environment roots and execution stacks to clean up memory, including memory-leaking cyclic object references.
* **Stack Overflow Protection:** Native call stack guards preventing C++ runtime segmentation faults by gracefully throwing JavaScript "Maximum call stack size exceeded" errors at a 200-depth limit.

### 6. Module System
* **Host Interop & Requires:** Implementation of NodeJS-style `require('fs')` and relative module importing (`require('./file.js')`). Enables reading and executing arbitrary host OS files.

---

<div align="center">
<i>Built with ❤️ for High-Performance Computing and Language Design.</i>
</div>
