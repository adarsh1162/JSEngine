#pragma once
#include "chunk.h"
#include "value.h"
#include "gc.h"
#include <unordered_map>
#include <string>

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * 256)

struct CallFrame {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
};

enum class InterpretResult {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
};

class VM {
public:
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Value stack[STACK_MAX];
    Value* stackTop;
    std::unordered_map<std::string, Value> globals;
    ObjUpvalue* openUpvalues;

    VM();
    ~VM();

    void resetStack();
    void push(Value value);
    Value pop();
    Value peek(int distance);

    bool call(ObjClosure* closure, int argCount);
    bool callValue(Value callee, int argCount);
    ObjUpvalue* captureUpvalue(Value* local);
    void closeUpvalues(Value* last);

    InterpretResult interpret(ObjFunction* function);
    InterpretResult run();
};
