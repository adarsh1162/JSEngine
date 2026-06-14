# JSEngine

JSEngine is a custom JavaScript engine written from scratch in C++ (C++17). It features an AST-walking Evaluator (V1) capable of interpreting advanced JavaScript concepts. An experimental Bytecode VM (V2) is also in early development within the source tree.

## Features Supported (V1 Evaluator)
- **Data Types:** Numbers, Strings, Booleans, Null, Undefined, Arrays, Objects, and Functions.
- **Language Constructs:** 
  - `if`/`else` control flow
  - `for` and `while` loops
  - Variable declarations (`let`, `const`)
- **Advanced Operators:**
  - Compound assignments (`+=`, `-=`, `*=`, `/=`)
  - Strict and loose equality (`===`, `!==`, `==`, `!=`), logical comparisons (`&&`, `||`, `!`), and `typeof`.
  - Spread syntax (`...arr`) and rest parameters (`...rest`) in function definitions.
- **Higher-Order Array Methods:** Native C++ implementations of `.map()`, `.filter()`, `.reduce()`, `.find()`, `.some()`, and `.every()` which execute JavaScript callback closures.
- **Closures & Scope:** Deep recursion and persistent variable closures via robust C++ environment modeling.
- **Built-ins:** Standard `Math` APIs (e.g. `random`, `max`, `min`, `pow`, `round`) and a mocked `Date` object constructor.

## Requirements

- **C++17 Compatible Compiler** (e.g., MinGW/GCC via MSYS2 on Windows, Clang on macOS, or GCC on Linux)
- A terminal with standard Unix-like shell tools (e.g., bash, zsh, or MSYS terminal).

## How to Compile

To build the project, run the following compilation command from the root of the project. If you are on Windows using MSYS2, make sure your MSYS path (like `C:\msys64\ucrt64\bin`) is correctly configured in your `PATH`.

```bash
g++ -std=c++17 src/*.cpp -o JSEngine.exe
```
*(On Linux/macOS, replace `JSEngine.exe` with `JSEngine`)*

## How to Run the Tests

Once compiled, you can run JavaScript files through the engine by passing the file path as an argument.

### Standard Tests
```bash
./JSEngine.exe tests/test1.js
./JSEngine.exe tests/test2.js
./JSEngine.exe tests/test3.js
./JSEngine.exe tests/test4.js
./JSEngine.exe tests/test5.js
```

## Interactive REPL Mode
If you run `JSEngine.exe` without any file arguments, it will open an interactive prompt where you can type JavaScript code directly:
```bash
./JSEngine.exe
```
*(Press `Ctrl+Z` on Windows or `Ctrl+D` on Linux/macOS to execute the snippet).*
