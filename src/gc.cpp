#include "gc.h"
#include <iostream>

Obj* objects = nullptr;

Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)::operator new(size);
    object->type = type;
    object->isMarked = false;
    object->next = objects;
    objects = object;
    return object;
}

ObjString* allocateString(const std::string& chars) {
    ObjString* string = (ObjString*)allocateObject(sizeof(ObjString), ObjType::OBJ_STRING);
    // Explicitly call the constructor for std::string inside the raw allocated memory
    new (&string->chars) std::string(chars);
    return string;
}

ObjFunction* allocateFunction() {
    ObjFunction* function = (ObjFunction*)allocateObject(sizeof(ObjFunction), ObjType::OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    new (&function->name) std::string("");
    new (&function->chunk) Chunk();
    return function;
}

#include "vm.h"
extern VM vm;

void markObject(Obj* object) {
    if (object == nullptr || object->isMarked) return;
    object->isMarked = true;
    
    if (object->type == ObjType::OBJ_FUNCTION) {
        ObjFunction* func = (ObjFunction*)object;
        for (Value val : func->chunk.constants) {
            markValue(val);
        }
    }
}

void markValue(Value value) {
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

void markRoots() {
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }
    for (auto& pair : vm.globals) {
        markValue(pair.second);
    }
}

void sweep() {
    Obj* previous = nullptr;
    Obj* object = objects;
    while (object != nullptr) {
        if (object->isMarked) {
            object->isMarked = false;
            previous = object;
            object = object->next;
        } else {
            Obj* unreached = object;
            object = object->next;
            if (previous != nullptr) {
                previous->next = object;
            } else {
                objects = object;
            }
            
            if (unreached->type == ObjType::OBJ_STRING) {
                ((ObjString*)unreached)->chars.~basic_string();
            } else if (unreached->type == ObjType::OBJ_FUNCTION) {
                ((ObjFunction*)unreached)->name.~basic_string();
                ((ObjFunction*)unreached)->chunk.~Chunk();
            }
            ::operator delete(unreached);
        }
    }
}

void collectGarbage() {
    markRoots();
    sweep();
}
