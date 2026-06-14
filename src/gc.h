#pragma once
#include "value.h"
#include "chunk.h"
#include <string>
#include <vector>
#include <unordered_map>

enum class ObjType {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_ARRAY
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

typedef Value (*NativeFn)(int argCount, Value* args);

struct ObjNative : public Obj {
    NativeFn function;
    std::string name;
};

struct ObjUpvalue : public Obj {
    Value* location;
    Value closed;
    ObjUpvalue* next;
};

struct ObjClosure : public Obj {
    ObjFunction* function;
    std::vector<ObjUpvalue*> upvalues;
};

struct ObjClass : public Obj {
    std::string name;
    std::unordered_map<std::string, Value> methods;
};

struct PropertyDescriptor {
    Value value;
    ObjFunction* get;
    ObjFunction* set;
    bool configurable;
    bool enumerable;
    bool writable;

    PropertyDescriptor() : value(UNDEFINED_VAL()), get(nullptr), set(nullptr),
                           configurable(true), enumerable(true), writable(true) {}
    PropertyDescriptor(Value val) : value(val), get(nullptr), set(nullptr),
                                    configurable(true), enumerable(true), writable(true) {}
};

struct ObjInstance : public Obj {
    ObjClass* klass;
    std::unordered_map<std::string, PropertyDescriptor> fields;
};

struct ObjArray : public Obj {
    std::vector<Value> elements;
};

// Allocators
ObjString* allocateString(const std::string& chars);
ObjFunction* allocateFunction();
ObjNative* allocateNative(NativeFn function, const std::string& name);
ObjClosure* allocateClosure(ObjFunction* function);
ObjUpvalue* allocateUpvalue(Value* slot);
ObjClass* allocateClass(const std::string& name);
ObjInstance* allocateInstance(ObjClass* klass);
ObjArray* allocateArray();

// Garbage Collection
void markObject(Obj* object);
void markValue(Value value);
void collectGarbage();

// Type macros
inline bool isObjType(Value value, ObjType type) { return IS_OBJ(value) && AS_OBJ(value)->type == type; }

#define IS_STRING(value)       isObjType(value, ObjType::OBJ_STRING)
#define IS_FUNCTION(value)     isObjType(value, ObjType::OBJ_FUNCTION)
#define IS_NATIVE(value)       isObjType(value, ObjType::OBJ_NATIVE)
#define IS_CLOSURE(value)      isObjType(value, ObjType::OBJ_CLOSURE)
#define IS_UPVALUE(value)      isObjType(value, ObjType::OBJ_UPVALUE)
#define IS_CLASS(value)        isObjType(value, ObjType::OBJ_CLASS)
#define IS_INSTANCE(value)     isObjType(value, ObjType::OBJ_INSTANCE)
#define IS_ARRAY(value)        isObjType(value, ObjType::OBJ_ARRAY)

#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value)       ((ObjNative*)AS_OBJ(value))
#define AS_CLOSURE(value)      ((ObjClosure*)AS_OBJ(value))
#define AS_UPVALUE(value)      ((ObjUpvalue*)AS_OBJ(value))
#define AS_CLASS(value)        ((ObjClass*)AS_OBJ(value))
#define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))
#define AS_ARRAY(value)        ((ObjArray*)AS_OBJ(value))

