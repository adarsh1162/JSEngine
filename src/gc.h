#pragma once
#include "value.h"
#include "chunk.h"
#include <string>
#include <vector>

enum class ObjType {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_CLOSURE,
    OBJ_UPVALUE
};

struct Obj {
    ObjType type;
    bool isMarked;
    Obj* next;
};

struct ObjString : public Obj {
    std::string chars;
};

struct ObjFunction : public Obj {
    int arity;
    int upvalueCount;
    Chunk chunk;
    std::string name;
};

ObjString* allocateString(const std::string& chars);
ObjFunction* allocateFunction();

void markObject(Obj* object);
void markValue(Value value);
void collectGarbage();
