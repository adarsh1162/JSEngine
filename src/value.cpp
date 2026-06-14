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
            if (obj->type == ObjType::OBJ_STRING) {
                std::cout << ((ObjString*)obj)->chars;
            } else {
                std::cout << "[Object]";
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
        case ValueType::VAL_OBJ:    return AS_OBJ(a) == AS_OBJ(b);
        default:                    return false; // Unreachable
    }
}
