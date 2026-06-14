#include "value.h"
#include "gc.h"
#include <iostream>

void printValue(Value value) {
    switch (value.type) {
        case ValueType::VAL_BOOL:
            std::cout << (AS_BOOL(value) ? "true" : "false");
            break;
        case ValueType::VAL_NIL:
            std::cout << "null";
            break;
        case ValueType::VAL_UNDEFINED:
            std::cout << "undefined";
            break;
        case ValueType::VAL_NUMBER:
            std::cout << AS_NUMBER(value);
            break;
        case ValueType::VAL_OBJ: {
            Obj* obj = AS_OBJ(value);
            switch (obj->type) {
                case ObjType::OBJ_STRING:
                    std::cout << ((ObjString*)obj)->chars;
                    break;
                case ObjType::OBJ_FUNCTION:
                    std::cout << "<fn " << ((ObjFunction*)obj)->name << ">";
                    break;
                case ObjType::OBJ_NATIVE:
                    std::cout << "<native fn>";
                    break;
                case ObjType::OBJ_CLOSURE:
                    std::cout << "<fn " << ((ObjClosure*)obj)->function->name << ">";
                    break;
                case ObjType::OBJ_UPVALUE:
                    std::cout << "upvalue";
                    break;
                case ObjType::OBJ_CLASS:
                    std::cout << "<class " << ((ObjClass*)obj)->name << ">";
                    break;
                case ObjType::OBJ_INSTANCE:
                    std::cout << "<" << ((ObjInstance*)obj)->klass->name << " instance>";
                    break;
                case ObjType::OBJ_ARRAY:
                    std::cout << "[Array]";
                    break;
            }
            break;
        }
    }
}

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case ValueType::VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case ValueType::VAL_NIL:    return true;
        case ValueType::VAL_UNDEFINED: return true;
        case ValueType::VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case ValueType::VAL_OBJ: {
            if (IS_STRING(a) && IS_STRING(b)) {
                return AS_STRING(a)->chars == AS_STRING(b)->chars;
            }
            return AS_OBJ(a) == AS_OBJ(b);
        }
        default:                    return false; // Unreachable
    }
}

bool isFalsey(Value value) {
    if (IS_NIL(value) || IS_UNDEFINED(value)) return true;
    if (IS_BOOL(value)) return !AS_BOOL(value);
    if (IS_NUMBER(value)) return AS_NUMBER(value) == 0;
    if (IS_STRING(value)) return AS_STRING(value)->chars.empty();
    return false;
}
