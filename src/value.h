#pragma once

enum class ValueType {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
    VAL_UNDEFINED
};

class Obj;

struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
    
    Value() : type(ValueType::VAL_UNDEFINED) { as.number = 0; }
};

inline Value BOOL_VAL(bool b) {
    Value v; v.type = ValueType::VAL_BOOL; v.as.boolean = b; return v;
}

inline Value NIL_VAL() {
    Value v; v.type = ValueType::VAL_NIL; v.as.number = 0; return v;
}

inline Value UNDEFINED_VAL() {
    Value v; v.type = ValueType::VAL_UNDEFINED; v.as.number = 0; return v;
}

inline Value NUMBER_VAL(double num) {
    Value v; v.type = ValueType::VAL_NUMBER; v.as.number = num; return v;
}

inline Value OBJ_VAL(Obj* obj) {
    Value v; v.type = ValueType::VAL_OBJ; v.as.obj = obj; return v;
}

#define IS_BOOL(value)    ((value).type == ValueType::VAL_BOOL)
#define IS_NIL(value)     ((value).type == ValueType::VAL_NIL)
#define IS_UNDEFINED(value) ((value).type == ValueType::VAL_UNDEFINED)
#define IS_NUMBER(value)  ((value).type == ValueType::VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == ValueType::VAL_OBJ)

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
#define AS_OBJ(value)     ((value).as.obj)

void printValue(Value value);
bool valuesEqual(Value a, Value b);
