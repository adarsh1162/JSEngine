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

ObjNative* allocateNative(NativeFn function, const std::string& name) {
    ObjNative* native = (ObjNative*)allocateObject(sizeof(ObjNative), ObjType::OBJ_NATIVE);
    native->function = function;
    new (&native->name) std::string(name);
    return native;
}

ObjClosure* allocateClosure(ObjFunction* function) {
    ObjClosure* closure = (ObjClosure*)allocateObject(sizeof(ObjClosure), ObjType::OBJ_CLOSURE);
    closure->function = function;
    new (&closure->upvalues) std::vector<ObjUpvalue*>();
    return closure;
}

ObjUpvalue* allocateUpvalue(Value* slot) {
    ObjUpvalue* upvalue = (ObjUpvalue*)allocateObject(sizeof(ObjUpvalue), ObjType::OBJ_UPVALUE);
    upvalue->location = slot;
    upvalue->closed = UNDEFINED_VAL();
    upvalue->next = nullptr;
    return upvalue;
}

ObjClass* allocateClass(const std::string& name) {
    ObjClass* klass = (ObjClass*)allocateObject(sizeof(ObjClass), ObjType::OBJ_CLASS);
    new (&klass->name) std::string(name);
    new (&klass->methods) std::unordered_map<std::string, Value>();
    return klass;
}

ObjInstance* allocateInstance(ObjClass* klass) {
    ObjInstance* instance = (ObjInstance*)allocateObject(sizeof(ObjInstance), ObjType::OBJ_INSTANCE);
    instance->klass = klass;
    new (&instance->fields) std::unordered_map<std::string, Value>();
    return instance;
}

ObjArray* allocateArray() {
    ObjArray* array = (ObjArray*)allocateObject(sizeof(ObjArray), ObjType::OBJ_ARRAY);
    new (&array->elements) std::vector<Value>();
    return array;
}

#include "vm.h"
extern VM vm;

void markObject(Obj* object) {
    if (object == nullptr || object->isMarked) return;
    object->isMarked = true;
    
    switch (object->type) {
        case ObjType::OBJ_FUNCTION: {
            ObjFunction* func = (ObjFunction*)object;
            for (Value val : func->chunk.constants) markValue(val);
            break;
        }
        case ObjType::OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            markObject((Obj*)closure->function);
            for (auto* upvalue : closure->upvalues) markObject((Obj*)upvalue);
            break;
        }
        case ObjType::OBJ_UPVALUE: {
            markValue(((ObjUpvalue*)object)->closed);
            break;
        }
        case ObjType::OBJ_CLASS: {
            ObjClass* klass = (ObjClass*)object;
            for (auto& pair : klass->methods) markValue(pair.second);
            break;
        }
        case ObjType::OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            markObject((Obj*)instance->klass);
            for (auto& pair : instance->fields) markValue(pair.second);
            break;
        }
        case ObjType::OBJ_ARRAY: {
            ObjArray* array = (ObjArray*)object;
            for (Value val : array->elements) markValue(val);
            break;
        }
        case ObjType::OBJ_STRING:
        case ObjType::OBJ_NATIVE:
            break;
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
                using String = std::string;
                ((ObjString*)unreached)->chars.~String();
            } else if (unreached->type == ObjType::OBJ_FUNCTION) {
                using String = std::string;
                ((ObjFunction*)unreached)->name.~String();
                ((ObjFunction*)unreached)->chunk.~Chunk();
            } else if (unreached->type == ObjType::OBJ_NATIVE) {
                using String = std::string;
                ((ObjNative*)unreached)->name.~String();
            } else if (unreached->type == ObjType::OBJ_CLOSURE) {
                using UpvalueVec = std::vector<ObjUpvalue*>;
                ((ObjClosure*)unreached)->upvalues.~UpvalueVec();
            } else if (unreached->type == ObjType::OBJ_CLASS) {
                using String = std::string;
                using Map = std::unordered_map<std::string, Value>;
                ((ObjClass*)unreached)->name.~String();
                ((ObjClass*)unreached)->methods.~Map();
            } else if (unreached->type == ObjType::OBJ_INSTANCE) {
                using Map = std::unordered_map<std::string, Value>;
                ((ObjInstance*)unreached)->fields.~Map();
            } else if (unreached->type == ObjType::OBJ_ARRAY) {
                using ValueVec = std::vector<Value>;
                ((ObjArray*)unreached)->elements.~ValueVec();
            }
            ::operator delete(unreached);
        }
    }
}

void collectGarbage() {
    markRoots();
    sweep();
}
