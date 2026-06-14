#pragma once
#include "chunk.h"
#include "value.h"
#include <unordered_map>
#include <string>

#define STACK_MAX 256

enum class InterpretResult {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
};

class VM {
public:
    Chunk* chunk;
    uint8_t* ip;
    Value stack[STACK_MAX];
    Value* stackTop;
    std::unordered_map<std::string, Value> globals;

    VM();
    ~VM();

    void resetStack();
    void push(Value value);
    Value pop();

    InterpretResult interpret(Chunk* chunk);
    InterpretResult run();
};
