<div align="center">
  <h1>JSEngine</h1>
  <h3>A highly robust, high-performance JavaScript engine built entirely from scratch in C++17.</h3>
  <p>JSEngine is a deeply embedded language runtime designed to parse, interpret, and execute JavaScript. It features a custom lexical tokenizer, a recursive descent AST parser, and a fully functional evaluation engine equipped with an asynchronous Event Loop, Deep Closures, Mark-and-Sweep Garbage Collection, and Industry-Ready Native Modules.</p>
</div>

---

## Introduction

Welcome to **JSEngine**! This project is an ambitious endeavor to reconstruct the core mechanics of a JavaScript runtime environment from the ground up, with **zero external dependencies** (no V8, SpiderMonkey, or QuickJS). 

Instead of relying on pre-existing interpreters, JSEngine implements every phase of code execution natively:
- **Lexical Analysis**: Converts raw string inputs into meaningful streams of tokens.
- **Syntactic Analysis (Parser)**: Constructs a highly detailed Abstract Syntax Tree (AST) representing complex logic.
- **Evaluation Engine**: Executes the AST while seamlessly managing memory, dynamic scopes, contexts, and closures.
- **Event Loop**: Accurately mimics the NodeJS non-blocking concurrency model via separate Macro and Microtask queues.

### The Dual-Engine Architecture (V1 vs V2)
This project contains two completely distinct execution architectures:

1. **V1 (Primary Engine - AST Evaluator)**: This is the heavily tested, highly robust main engine of this repository. It recursively evaluates the AST in real-time. It is built for 100% specification accuracy, deep closures, and comprehensive built-in APIs. **You should always use V1 as the primary engine.**
2. **V2 (Experimental Bytecode Engine & VM)**: The V2 engine abandons the AST evaluator and instead introduces a **Bytecode Compiler and Virtual Machine (VM)**. The AST is compiled into low-level operational bytecode which is then executed via a blazing fast dispatch loop in the VM. **The purpose of V2** is extreme performance, optimizing execution speeds by eliminating the overhead of recursively traversing tree nodes during runtime.

---

## Comprehensive Features 

### V1 (Primary AST Engine)
The V1 engine brings the essential mechanics of JavaScript to life. It has been extensively upgraded to solve profound language limitations and simulate a **Production-Ready NodeJS Environment**:

#### Core Language Capabilities
- **Variables & Scope**: Complete implementation of `let`, `const`, `var`, strict block scoping, and Temporal Dead Zones (TDZ).
- **Data Types**: Native implementation of `Number`, `String`, `Boolean`, `Null`, and `Undefined`.
- **Advanced Arrays**: Dynamic array allocation, bracket notation access (`arr[0]`), push/pop operations, dynamic `length` tracking, and high-order iteration methods (`forEach`, `findIndex`).
- **Objects & Hashes**: Key-value pair hashing, nested objects, dot notation (`obj.key`), bracket notation (`obj["key"]`) resolution, and static methods (`Object.keys`, `Object.values`, `Object.entries`, `Object.assign`).
- **Collections**: Native polyfill support for `Map` and `Set` for high-performance unique data storage.

#### Execution & Architecture
- **Functions & Execution Contexts**: First-class citizens! Function declarations, anonymous functions, and arrow functions (`() => {}`).
- **Deep Closures**: Functions accurately capture and retain their lexical environment scopes. No variable is left behind!
- **Context Binding**: Native `this` keyword resolution and automatic `arguments` object injection.
- **Control Flow Mechanisms**: Deep implementations of `if/else`, `while`, `do-while`, and `for` loops with Control jumps (nested `break`, `continue`, `return`).
- **Object-Oriented Programming (OOP)**: Complete ES6 `class` syntax support including Constructors, Methods, Prototype chain delegation, Inheritance (`extends`), and `super` calls.

#### Concurrency, Async & Multi-Threading
- **Asynchronous Event Loop**: Completely mimics NodeJS's non-blocking concurrency model! 
  - **Macrotasks**: `setTimeout`, `setInterval`, `clearTimeout`, `clearInterval`.
  - **Microtasks**: Native `Promise` execution, `Promise.all`, and `queueMicrotask`. 
- **True Multi-Threading (Worker API)**: Spawn parallel execution threads using a Native `Worker` API. Messages are passed safely between C++ `std::thread` boundaries avoiding Event Loop blockage.

#### Robust System & Memory Management
- **Garbage Collection**: Automated **Mark-and-Sweep Memory Management**. Successfully tracks, isolates, and cleans up cyclical references without memory leaks!
- **Error Handling**: Comprehensive `try...catch...finally` blocks with Native JS Catch block wrapping (`e.name`, `e.message`).
- **Stack Traces**: Custom ExecutionStackGuard for precise and deep stack logs during Runtime Exceptions.

---

## Next-Level Native APIs (NEW)
JSEngine has evolved from a toy interpreter to a **fully-fledged backend runtime**. It now features highly optimized native bindings for advanced capabilities:

### 1. Native Hardware-Accelerated Cryptography (`crypto`)
Utilizes Windows `<bcrypt.h>` and `<wincrypt.h>` to provide zero-dependency, ultra-fast cryptographic hashing directly in JavaScript.
```javascript
let hash = crypto.createHash("sha256");
hash.update("Hello JSEngine!");
console.log(hash.digest("hex")); // Prints lightning-fast SHA-256 hash!
```

### 2. Real-Time File System Watcher (`fs.watch`)
Leverages Windows `ReadDirectoryChangesW` hooked into our Event Loop via thread-safe queues. This enables tools like `nodemon` or hot-reloading architectures purely in JSEngine.
```javascript
fs.watch(process.cwd(), (event, filename) => {
    console.log("Hot-reload triggered! File changed: " + filename);
});
```

### 3. Embedded SQLite Database (`sqlite`)
We embedded the world-renowned `sqlite3.c` amalgamation engine. JSEngine provides a robust, disk-backed SQL database right out of the box with synchronous querying mapped straight to native JS objects.
```javascript
let db = new sqlite.Database("test.db");
db.exec("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)");
db.exec("INSERT INTO users (name) VALUES ('Adarsh')");
let rows = db.query("SELECT * FROM users");
console.log(rows[0].name); // "Adarsh"
```

### 4. Full Native WebSocket Server (`ws`)
Supports bidirectional, real-time TCP sockets with HTTP Upgrade Handshakes (using SHA-1 and Base64 encryption) and RFC 6455 packet framing. Build real-time chat apps seamlessly!
```javascript
let server = new ws.WebSocketServer({ port: 8080 });
server.on("connection", (socket) => {
    socket.on("message", (msg) => {
        socket.send("Echo: " + msg);
    });
});
```

### 5. Standalone Binary Packager (`--compile`)
Inspired by Deno and Bun, JSEngine now features a built-in packager that bundles your JavaScript applications into standalone, highly portable `.exe` binaries using a blazing-fast payload append method!
```bash
# Compile your JS file into a standalone Windows Executable!
JSEngine.exe --compile my_app.js -o built_app.exe
./built_app.exe
```

### 6. NPM / CommonJS Module Resolution
JSEngine completely overhauls the basic `require()` function to mimic NodeJS's legendary module resolution algorithm. It natively parses `package.json`, traverses `node_modules`, handles cyclic dependencies with a `requireCache`, and automatically injects proper `__dirname` and `__filename` into module scopes!
```javascript
// Natively consume external packages from node_modules!
const express = require("express"); 
const myLocalLib = require("./src/my_lib.js");
```

---

### V2 (Bytecode Compiler & VM)
The V2 Experimental Engine takes a performance-first approach, dropping recursive AST execution in favor of a Stack-Based Virtual Machine:

- **Bytecode Compiler**: Walks the Abstract Syntax Tree exactly once and translates nodes into a dense sequence of Operation Codes (OpCodes).
- **Virtual Machine (VM)**: A highly optimized, linear loop-based execution runtime.
- **Stack-Based Architecture**: Eliminates heavy recursive function calls. Operands and results are pushed/popped from an extremely fast linear stack structure (Push, Pop, Add, Sub, Call, LoadVar, StoreVar).
- **Extreme Performance**: Outperforms the AST engine in loop-heavy and CPU-intensive operations by removing memory-heavy tree traversals during execution.
- **Lexical Environment Storage**: Flattens scopes and registers variables linearly, making scope resolutions O(1) in many cases compared to deep AST hash map lookups.

---

## Getting Started (How to Compile & Run)

Because JSEngine is written in raw C++17, it is completely cross-platform natively. **However, the new Next-Level Native APIs (Crypto, FS Watcher, SQLite, WebSockets) are currently highly optimized for Windows (MSYS2 / MinGW-w64)**.

### Windows (Using MSYS2 / MinGW-w64) - Full Features Supported
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
   # Step A: Compile SQLite amalgamation
   gcc -c src/sqlite3.c -o src/sqlite3.o
   
   # Step B: Compile JSEngine & Link Dependencies
   g++ -std=c++17 src/*.cpp src/sqlite3.o -o JSEngine.exe -lws2_32 -lbCRYPT -lcrypt32 -pthread
   
   # Step C: Run it!
   ./JSEngine.exe tests/advanced_upgrades.js
   
   # ========================================
   # Run V2 (Bytecode Engine / VM)
   # ========================================
   g++ -std=c++17 compiler.cpp vm.o -o JSEngineV2.exe
   ./JSEngineV2.exe tests/advanced_upgrades.js
   ```

### Linux (Ubuntu / Debian) - Core V1 & V2 Only
The Native APIs are Windows-specific. To run the core AST or VM engines on Linux:
1. Install the `build-essential` package:
   ```bash
   sudo apt update
   sudo apt install build-essential
   ```
2. Compile and Run:
   ```bash
   # ========================================
   # Run V1 (Primary AST Engine)
   # ========================================
   g++ -std=c++17 src/*.cpp -o JSEngine -pthread
   ./JSEngine tests/advanced_upgrades.js

   # ========================================
   # Run V2 (Bytecode Engine / VM)
   # ========================================
   g++ -std=c++17 compiler.cpp vm.o -o JSEngineV2
   ./JSEngineV2 tests/advanced_upgrades.js
   ```

### macOS - Core V1 & V2 Only
1. Install Xcode Command Line Tools:
   ```bash
   xcode-select --install
   ```
2. Compile and Run:
   ```bash
   # ========================================
   # Run V1 (Primary AST Engine)
   # ========================================
   g++ -std=c++17 src/*.cpp -o JSEngine -pthread
   ./JSEngine tests/advanced_upgrades.js

   # ========================================
   # Run V2 (Bytecode Engine / VM)
   # ========================================
   g++ -std=c++17 compiler.cpp vm.o -o JSEngineV2
   ./JSEngineV2 tests/advanced_upgrades.js
   ```

### Running the Feature Tests (Windows Only)
We have included dedicated test scripts to demonstrate the power of the new Native APIs!

**1. Run Native Crypto Test:**
```bash
./JSEngine.exe tests/test_crypto.js
```

**2. Run Native SQLite Test:**
```bash
./JSEngine.exe tests/test_sqlite.js
```

**3. Run File System Watcher Test:**
```bash
./JSEngine.exe tests/test_fs_watch.js
```

**4. Run WebSocket Server Test:**
```bash
./JSEngine.exe tests/test_ws.js
```
*(After starting the WebSocket Server, open your Browser Console and run `let ws = new WebSocket("ws://localhost:8080"); ws.onmessage = e => console.log(e.data); ws.send("Hello JSEngine!");` to see the magic happen!)*

---

## Project Structure

```text
JSEngine/
├── src/
│   ├── main.cpp         # CLI Entry Point (V1)
│   ├── lexer.cpp        # Lexical Tokenizer
│   ├── parser.cpp       # AST Construction
│   ├── evaluator.cpp    # AST Execution & Event Loop
│   ├── builtins.cpp     # Native APIs (console, Math)
│   ├── worker_api.cpp   # Multi-Threading Native Workers
│   ├── crypto_api.cpp   # Native Cryptography 
│   ├── net_ws.cpp       # Native WebSocket Server
│   ├── sqlite_api.cpp   # Embedded SQLite Database 
│   └── gc.h             # Mark-and-Sweep Garbage Collector
├── tests/
│   ├── test_crypto.js   # Crypto API Test
│   ├── test_sqlite.js   # SQLite Test
│   ├── test_ws.js       # WebSocket Test
│   ├── test_fs_watch.js # File System Watcher Test
│   └── advanced_upgrades.js  # V1 Integration & Stress Tests
└── README.md
```

---

<div align="center">
  <i>Engineered with ❤️ by Adarsh</i>
</div>
