import re

with open('src/vm.cpp', 'r') as f:
    c = f.read()

c = c.replace(
    'if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) return InterpretResult::INTERPRET_RUNTIME_ERROR;',
    'if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { runtimeError("Operands must be numbers."); return InterpretResult::INTERPRET_RUNTIME_ERROR; }'
)

c = c.replace(
    'if (!IS_NUMBER(peek(0))) return InterpretResult::INTERPRET_RUNTIME_ERROR;',
    'if (!IS_NUMBER(peek(0))) { runtimeError("Operand must be a number."); return InterpretResult::INTERPRET_RUNTIME_ERROR; }'
)

c = c.replace(
    'if (!IS_INSTANCE(peek(0))) return InterpretResult::INTERPRET_RUNTIME_ERROR;',
    'if (!IS_INSTANCE(peek(0))) { runtimeError("Only instances have properties."); return InterpretResult::INTERPRET_RUNTIME_ERROR; }'
)

c = c.replace(
    'if (!IS_INSTANCE(peek(1))) return InterpretResult::INTERPRET_RUNTIME_ERROR;',
    'if (!IS_INSTANCE(peek(1))) { runtimeError("Only instances have properties."); return InterpretResult::INTERPRET_RUNTIME_ERROR; }'
)

c = re.sub(
    r'return InterpretResult::INTERPRET_RUNTIME_ERROR;',
    r'runtimeError("Runtime error."); return InterpretResult::INTERPRET_RUNTIME_ERROR;',
    c
)

# Fix double runtimeErrors if we replaced twice
c = c.replace('runtimeError("Runtime error."); runtimeError', 'runtimeError')
c = c.replace('runtimeError("Operands must be numbers."); runtimeError("Runtime error.");', 'runtimeError("Operands must be numbers.");')
c = c.replace('runtimeError("Operand must be a number."); runtimeError("Runtime error.");', 'runtimeError("Operand must be a number.");')
c = c.replace('runtimeError("Only instances have properties."); runtimeError("Runtime error.");', 'runtimeError("Only instances have properties.");')


with open('src/vm.cpp', 'w') as f:
    f.write(c)
