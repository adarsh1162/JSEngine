import re

with open('src/gc.cpp', 'r') as f:
    c = f.read()

c = c.replace('((ObjInstance*)unreached)->fields.~Map();', 'using InstanceMap = std::unordered_map<std::string, PropertyDescriptor>; ((ObjInstance*)unreached)->fields.~InstanceMap();')

with open('src/gc.cpp', 'w') as f:
    f.write(c)


with open('src/vm.cpp', 'r') as f:
    c = f.read()

c = c.replace(
'''            case static_cast<uint8_t>(OpCode::OP_GET_GLOBAL): {
                ObjString* name = AS_STRING(READ_CONSTANT());
                auto it = globals.find(name->chars);
                if (it == globals.end()) {
                    std::cerr << "Undefined variable '" << name->chars << "'.\\n";
                    runtimeError("Runtime error."); return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                push(it->second.value);
                break;
            }''',
'''            case static_cast<uint8_t>(OpCode::OP_GET_GLOBAL): {
                ObjString* name = AS_STRING(READ_CONSTANT());
                auto it = globals.find(name->chars);
                if (it == globals.end()) {
                    std::cerr << "Undefined variable '" << name->chars << "'.\\n";
                    runtimeError("Runtime error."); return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                push(it->second);
                break;
            }'''
)

with open('src/vm.cpp', 'w') as f:
    f.write(c)
