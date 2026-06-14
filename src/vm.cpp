#include "vm.h"
#include "gc.h"
#include <iostream>
#include <cmath>

VM::VM() {
    resetStack();
}

VM::~VM() {}

void VM::resetStack() {
    stackTop = stack;
}

void VM::push(Value value) {
    *stackTop = value;
    stackTop++;
}

Value VM::pop() {
    stackTop--;
    return *stackTop;
}

InterpretResult VM::interpret(Chunk* chunk) {
    this->chunk = chunk;
    this->ip = chunk->code.data();
    resetStack();
    push(UNDEFINED_VAL()); // Dummy value for slot 0
    return run();
}

InterpretResult VM::run() {
#define READ_BYTE() (*ip++)
#define READ_CONSTANT() (chunk->constants[READ_BYTE()])
#define BINARY_OP(valueType, op) \
    do { \
        if (!IS_NUMBER(*(stackTop - 1)) || !IS_NUMBER(*(stackTop - 2))) { \
            std::cerr << "Operands must be numbers.\n"; \
            return InterpretResult::INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
    } while (false)

    while (true) {
        // Optional debugging: print stack trace
        /*
        std::cout << "          ";
        for (Value* slot = stack; slot < stackTop; slot++) {
            std::cout << "[ ";
            printValue(*slot);
            std::cout << " ]";
        }
        std::cout << "\n";
        chunk->disassembleInstruction((int)(ip - chunk->code.data()));
        */

        uint8_t instruction = READ_BYTE();
        switch (static_cast<OpCode>(instruction)) {
            case OpCode::OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OpCode::OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(stack[slot]);
                break;
            }
            case OpCode::OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                stack[slot] = *(stackTop - 1);
                break;
            }
            case OpCode::OP_GET_GLOBAL: {
                ObjString* name = (ObjString*)AS_OBJ(READ_CONSTANT());
                if (globals.find(name->chars) == globals.end()) {
                    std::cerr << "Undefined variable '" << name->chars << "'.\n";
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                push(globals[name->chars]);
                break;
            }
            case OpCode::OP_DEFINE_GLOBAL: {
                ObjString* name = (ObjString*)AS_OBJ(READ_CONSTANT());
                globals[name->chars] = pop();
                break;
            }
            case OpCode::OP_SET_GLOBAL: {
                ObjString* name = (ObjString*)AS_OBJ(READ_CONSTANT());
                if (globals.find(name->chars) == globals.end()) {
                    std::cerr << "Undefined variable '" << name->chars << "'.\n";
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                globals[name->chars] = *(stackTop - 1);
                break;
            }
            case OpCode::OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OpCode::OP_LESS: {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(BOOL_VAL(a < b));
                break;
            }
            case OpCode::OP_LESS_EQUAL: {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(BOOL_VAL(a <= b));
                break;
            }
            case OpCode::OP_GREATER: {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(BOOL_VAL(a > b));
                break;
            }
            case OpCode::OP_GREATER_EQUAL: {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(BOOL_VAL(a >= b));
                break;
            }
            case OpCode::OP_MODULO: {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(std::fmod(a, b)));
                break;
            }
            case OpCode::OP_ADD: {
                if (IS_NUMBER(*(stackTop - 1)) && IS_NUMBER(*(stackTop - 2))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    Value b = pop();
                    Value a = pop();
                    std::string strA = "";
                    if (IS_NUMBER(a)) {
                        strA = std::to_string(AS_NUMBER(a));
                        strA.erase(strA.find_last_not_of('0') + 1, std::string::npos);
                        if (strA.back() == '.') strA.pop_back();
                    } else if (IS_OBJ(a) && AS_OBJ(a)->type == ObjType::OBJ_STRING) {
                        strA = ((ObjString*)AS_OBJ(a))->chars;
                    } else if (IS_BOOL(a)) {
                        strA = AS_BOOL(a) ? "true" : "false";
                    }
                    
                    std::string strB = "";
                    if (IS_NUMBER(b)) {
                        strB = std::to_string(AS_NUMBER(b));
                        strB.erase(strB.find_last_not_of('0') + 1, std::string::npos);
                        if (strB.back() == '.') strB.pop_back();
                    } else if (IS_OBJ(b) && AS_OBJ(b)->type == ObjType::OBJ_STRING) {
                        strB = ((ObjString*)AS_OBJ(b))->chars;
                    } else if (IS_BOOL(b)) {
                        strB = AS_BOOL(b) ? "true" : "false";
                    }
                    push(OBJ_VAL(allocateString(strA + strB)));
                }
                break;
            }
            case OpCode::OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OpCode::OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OpCode::OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
            case OpCode::OP_JUMP: {
                uint16_t offset = (uint16_t)(READ_BYTE() << 8);
                offset |= READ_BYTE();
                ip += offset;
                break;
            }
            case OpCode::OP_JUMP_IF_FALSE: {
                uint16_t offset = (uint16_t)(READ_BYTE() << 8);
                offset |= READ_BYTE();
                Value condition = *(stackTop - 1);
                bool isFalse = IS_BOOL(condition) ? !AS_BOOL(condition) : (IS_NIL(condition) || IS_UNDEFINED(condition) || (IS_NUMBER(condition) && AS_NUMBER(condition) == 0));
                if (isFalse) ip += offset;
                break;
            }
            case OpCode::OP_LOOP: {
                uint16_t offset = (uint16_t)(READ_BYTE() << 8);
                offset |= READ_BYTE();
                ip -= offset;
                break;
            }
            case OpCode::OP_PRINT: {
                printValue(pop());
                std::cout << "\n";
                push(UNDEFINED_VAL());
                break;
            }
            case OpCode::OP_NEGATE: {
                if (!IS_NUMBER(*(stackTop - 1))) {
                    std::cerr << "Operand must be a number.\n";
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }
            case OpCode::OP_POP: {
                pop();
                break;
            }
            case OpCode::OP_RETURN: {
                return InterpretResult::INTERPRET_OK;
            }
            default:
                std::cerr << "Unknown opcode: " << (int)instruction << "\n";
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}
