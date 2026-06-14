#include "vm.h"
#include <iostream>
#include <cmath>

VM::VM() {
    resetStack();
}

VM::~VM() {}

void VM::resetStack() {
    stackTop = stack;
    frameCount = 0;
    openUpvalues = nullptr;
}

void VM::push(Value value) {
    *stackTop = value;
    stackTop++;
}

Value VM::pop() {
    stackTop--;
    return *stackTop;
}

Value VM::peek(int distance) {
    return stackTop[-1 - distance];
}

bool VM::call(ObjClosure* closure, int argCount) {
    if (argCount != closure->function->arity) {
        std::cerr << "Expected " << closure->function->arity << " arguments but got " << argCount << ".\n";
        return false;
    }

    if (frameCount == FRAMES_MAX) {
        std::cerr << "Stack overflow.\n";
        return false;
    }

    CallFrame* frame = &frames[frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code.data();
    frame->slots = stackTop - argCount - 1;
    return true;
}

bool VM::callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (AS_OBJ(callee)->type) {
            case ObjType::OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), argCount);
            case ObjType::OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee)->function;
                Value result = native(argCount, stackTop - argCount);
                stackTop -= argCount + 1;
                push(result);
                return true;
            }
            default:
                break; // Non-callable object type
        }
    }
    std::cerr << "Can only call functions and classes.\n";
    return false;
}

ObjUpvalue* VM::captureUpvalue(Value* local) {
    ObjUpvalue* prevUpvalue = nullptr;
    ObjUpvalue* upvalue = openUpvalues;
    while (upvalue != nullptr && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != nullptr && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = allocateUpvalue(local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == nullptr) {
        openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

void VM::closeUpvalues(Value* last) {
    while (openUpvalues != nullptr && openUpvalues->location >= last) {
        ObjUpvalue* upvalue = openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        openUpvalues = upvalue->next;
    }
}

InterpretResult VM::interpret(ObjFunction* function) {
    resetStack();
    ObjClosure* closure = allocateClosure(function);
    push(OBJ_VAL(closure));
    call(closure, 0);
    return run();
}

InterpretResult VM::run() {
    CallFrame* frame = &frames[frameCount - 1];
    
    #define READ_BYTE() (*frame->ip++)
    #define READ_CONSTANT() (frame->closure->function->chunk.constants[READ_BYTE()])

    for (;;) {
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case static_cast<uint8_t>(OpCode::OP_CONSTANT): {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_NIL): push(NIL_VAL()); break;
            case static_cast<uint8_t>(OpCode::OP_TRUE): push(BOOL_VAL(true)); break;
            case static_cast<uint8_t>(OpCode::OP_FALSE): push(BOOL_VAL(false)); break;
            case static_cast<uint8_t>(OpCode::OP_POP): pop(); break;
            
            case static_cast<uint8_t>(OpCode::OP_GET_LOCAL): {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_SET_LOCAL): {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_GET_GLOBAL): {
                ObjString* name = AS_STRING(READ_CONSTANT());
                auto it = globals.find(name->chars);
                if (it == globals.end()) {
                    std::cerr << "Undefined variable '" << name->chars << "'.\n";
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                push(it->second);
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_DEFINE_GLOBAL): {
                ObjString* name = AS_STRING(READ_CONSTANT());
                globals[name->chars] = peek(0);
                pop();
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_SET_GLOBAL): {
                ObjString* name = AS_STRING(READ_CONSTANT());
                if (globals.find(name->chars) == globals.end()) {
                    std::cerr << "Undefined variable '" << name->chars << "'.\n";
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                globals[name->chars] = peek(0);
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_GET_UPVALUE): {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_SET_UPVALUE): {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_EQUAL): {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_GREATER): {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) return InterpretResult::INTERPRET_RUNTIME_ERROR;
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(BOOL_VAL(a > b));
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_LESS): {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) return InterpretResult::INTERPRET_RUNTIME_ERROR;
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(BOOL_VAL(a < b));
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_GREATER_EQUAL): {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) return InterpretResult::INTERPRET_RUNTIME_ERROR;
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(BOOL_VAL(a >= b));
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_LESS_EQUAL): {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) return InterpretResult::INTERPRET_RUNTIME_ERROR;
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(BOOL_VAL(a <= b));
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_ADD): {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    ObjString* b = AS_STRING(pop());
                    ObjString* a = AS_STRING(pop());
                    push(OBJ_VAL(allocateString(a->chars + b->chars)));
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_SUBTRACT): {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) return InterpretResult::INTERPRET_RUNTIME_ERROR;
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a - b));
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_MULTIPLY): {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) return InterpretResult::INTERPRET_RUNTIME_ERROR;
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a * b));
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_DIVIDE): {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) return InterpretResult::INTERPRET_RUNTIME_ERROR;
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a / b));
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_MODULO): {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) return InterpretResult::INTERPRET_RUNTIME_ERROR;
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(std::fmod(a, b)));
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_NOT):
                push(BOOL_VAL(IS_FALSEY(pop())));
                break;
            case static_cast<uint8_t>(OpCode::OP_NEGATE): {
                if (!IS_NUMBER(peek(0))) return InterpretResult::INTERPRET_RUNTIME_ERROR;
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_PRINT): {
                printValue(pop());
                std::cout << std::endl;
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_JUMP): {
                uint16_t offset = (frame->ip[0] << 8) | frame->ip[1];
                frame->ip += 2;
                frame->ip += offset;
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_JUMP_IF_FALSE): {
                uint16_t offset = (frame->ip[0] << 8) | frame->ip[1];
                frame->ip += 2;
                if (IS_FALSEY(peek(0))) frame->ip += offset;
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_LOOP): {
                uint16_t offset = (frame->ip[0] << 8) | frame->ip[1];
                frame->ip += 2;
                frame->ip -= offset;
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_CALL): {
                int argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount)) {
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                frame = &frames[frameCount - 1];
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_CLOSURE): {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = allocateClosure(function);
                push(OBJ_VAL(closure));
                
                for (int i = 0; i < closure->function->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues.push_back(captureUpvalue(frame->slots + index));
                    } else {
                        closure->upvalues.push_back(frame->closure->upvalues[index]);
                    }
                }
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_CLOSE_UPVALUE): {
                closeUpvalues(stackTop - 1);
                pop();
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_RETURN): {
                Value result = pop();
                closeUpvalues(frame->slots);
                frameCount--;
                if (frameCount == 0) {
                    pop();
                    return InterpretResult::INTERPRET_OK;
                }
                
                stackTop = frame->slots;
                push(result);
                frame = &frames[frameCount - 1];
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_BUILD_ARRAY): {
                int count = READ_BYTE();
                ObjArray* array = allocateArray();
                for (int i = count - 1; i >= 0; i--) {
                    array->elements.insert(array->elements.begin(), pop());
                }
                push(OBJ_VAL(array));
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_BUILD_OBJECT): {
                int count = READ_BYTE();
                ObjInstance* obj = allocateInstance(nullptr);
                for (int i = 0; i < count; i++) {
                    Value value = pop();
                    ObjString* key = AS_STRING(pop());
                    obj->fields[key->chars] = value;
                }
                push(OBJ_VAL(obj));
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_GET_PROPERTY): {
                if (!IS_INSTANCE(peek(0))) return InterpretResult::INTERPRET_RUNTIME_ERROR;
                ObjInstance* instance = AS_INSTANCE(peek(0));
                ObjString* name = AS_STRING(READ_CONSTANT());
                
                auto it = instance->fields.find(name->chars);
                if (it != instance->fields.end()) {
                    pop(); // Instance
                    push(it->second);
                    break;
                }
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
            }
            case static_cast<uint8_t>(OpCode::OP_SET_PROPERTY): {
                if (!IS_INSTANCE(peek(1))) return InterpretResult::INTERPRET_RUNTIME_ERROR;
                ObjInstance* instance = AS_INSTANCE(peek(1));
                ObjString* name = AS_STRING(READ_CONSTANT());
                instance->fields[name->chars] = peek(0);
                Value value = pop();
                pop(); // Instance
                push(value);
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_ARRAY_GET): {
                Value property = pop();
                Value object = pop();
                if (IS_ARRAY(object) && IS_NUMBER(property)) {
                    ObjArray* arr = AS_ARRAY(object);
                    int index = AS_NUMBER(property);
                    if (index >= 0 && index < arr->elements.size()) {
                        push(arr->elements[index]);
                    } else push(UNDEFINED_VAL());
                } else if (IS_INSTANCE(object) && IS_STRING(property)) {
                    ObjInstance* inst = AS_INSTANCE(object);
                    std::string key = AS_STRING(property)->chars;
                    auto it = inst->fields.find(key);
                    if (it != inst->fields.end()) push(it->second);
                    else push(UNDEFINED_VAL());
                } else {
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_ARRAY_SET): {
                Value value = pop();
                Value property = pop();
                Value object = pop();
                if (IS_ARRAY(object) && IS_NUMBER(property)) {
                    ObjArray* arr = AS_ARRAY(object);
                    int index = AS_NUMBER(property);
                    if (index >= 0) {
                        if (index >= arr->elements.size()) arr->elements.resize(index + 1, UNDEFINED_VAL());
                        arr->elements[index] = value;
                        push(value);
                    } else return InterpretResult::INTERPRET_RUNTIME_ERROR;
                } else if (IS_INSTANCE(object) && IS_STRING(property)) {
                    ObjInstance* inst = AS_INSTANCE(object);
                    std::string key = AS_STRING(property)->chars;
                    inst->fields[key] = value;
                    push(value);
                } else {
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
        }
    }
}
