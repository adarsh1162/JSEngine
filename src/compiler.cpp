#include "compiler.h"
#include "gc.h"
#include <iostream>

Compiler::Compiler(Compiler* enclosing, FunctionType type, const std::string& name)
    : enclosing(enclosing), type(type), localCount(0), scopeDepth(0) {
    function = allocateFunction();
    if (!name.empty()) {
        function->name = name;
    }
    
    Local& local = locals[localCount++];
    local.depth = 0;
    local.isCaptured = false;
    local.name = (type == FunctionType::TYPE_FUNCTION) ? "" : "this";
}

ObjFunction* Compiler::endCompiler() {
    emitReturn();
    return function;
}

Chunk* Compiler::currentChunk() {
    return &function->chunk;
}

void Compiler::emitByte(uint8_t byte) {
    currentChunk()->write(byte, 1);
}

void Compiler::emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

void Compiler::emitConstant(Value value) {
    int constant = currentChunk()->addConstant(value);
    if (constant > 255) throw std::runtime_error("Too many constants in one chunk.");
    emitBytes(static_cast<uint8_t>(OpCode::OP_CONSTANT), constant);
}

int Compiler::emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->code.size() - 2;
}

void Compiler::patchJump(int offset) {
    int jump = currentChunk()->code.size() - offset - 2;
    if (jump > 65535) throw std::runtime_error("Too much code to jump over.");
    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

void Compiler::emitLoop(int loopStart) {
    emitByte(static_cast<uint8_t>(OpCode::OP_LOOP));
    int jump = currentChunk()->code.size() - loopStart + 2;
    if (jump > 65535) throw std::runtime_error("Loop body too large.");
    emitByte((jump >> 8) & 0xff);
    emitByte(jump & 0xff);
}

void Compiler::emitReturn() {
    emitByte(static_cast<uint8_t>(OpCode::OP_NIL));
    emitByte(static_cast<uint8_t>(OpCode::OP_RETURN));
}

void Compiler::beginScope() {
    scopeDepth++;
}

void Compiler::endScope() {
    scopeDepth--;
    while (localCount > 0 && locals[localCount - 1].depth > scopeDepth) {
        if (locals[localCount - 1].isCaptured) {
            emitByte(static_cast<uint8_t>(OpCode::OP_CLOSE_UPVALUE));
        } else {
            emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        }
        localCount--;
    }
}

void Compiler::addLocal(const std::string& name) {
    if (localCount == 256) throw std::runtime_error("Too many local variables in function.");
    Local& local = locals[localCount++];
    local.name = name;
    local.depth = scopeDepth;
    local.isCaptured = false;
}

int Compiler::resolveLocal(const std::string& name) {
    for (int i = localCount - 1; i >= 0; i--) {
        if (locals[i].name == name) {
            return i;
        }
    }
    return -1;
}

int Compiler::resolveUpvalue(const std::string& name) {
    if (enclosing == nullptr) return -1;
    
    int local = enclosing->resolveLocal(name);
    if (local != -1) {
        enclosing->locals[local].isCaptured = true;
        return addUpvalue((uint8_t)local, true);
    }
    
    int upvalue = enclosing->resolveUpvalue(name);
    if (upvalue != -1) {
        return addUpvalue((uint8_t)upvalue, false);
    }
    
    return -1;
}

int Compiler::addUpvalue(uint8_t index, bool isLocal) {
    int upvalueCount = function->upvalueCount;
    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }
    if (upvalueCount == 256) throw std::runtime_error("Too many closure variables.");
    upvalues[upvalueCount].isLocal = isLocal;
    upvalues[upvalueCount].index = index;
    return function->upvalueCount++;
}


// --- AstCompiler ---

AstCompiler::AstCompiler() : current(nullptr) {}

ObjFunction* AstCompiler::compile(std::shared_ptr<Program> program) {
    Compiler compiler(nullptr, FunctionType::TYPE_SCRIPT, "");
    current = &compiler;
    
    try {
        for (auto stmt : program->body) {
            compileStatement(stmt);
        }
        ObjFunction* function = current->endCompiler();
        current = current->enclosing;
        return function;
    } catch (const std::exception& e) {
        std::cerr << "Compile error: " << e.what() << "\n";
        current = current->enclosing;
        return nullptr;
    }
}

void AstCompiler::compileStatement(std::shared_ptr<Statement> stmt) {
    if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
        compileExpression(exprStmt->expression);
        current->emitByte(static_cast<uint8_t>(OpCode::OP_POP));
    }
    else if (auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(stmt)) {
        if (varDecl->initializer) {
            compileExpression(varDecl->initializer);
        } else {
            current->emitByte(static_cast<uint8_t>(OpCode::OP_NIL));
        }
        
        if (current->scopeDepth > 0) {
            current->addLocal(varDecl->name);
        } else {
            ObjString* nameStr = allocateString(varDecl->name);
            int constant = current->currentChunk()->addConstant(OBJ_VAL(nameStr)); 
            current->emitBytes(static_cast<uint8_t>(OpCode::OP_DEFINE_GLOBAL), constant);
        }
    }
    else if (auto funDecl = std::dynamic_pointer_cast<FunctionDeclaration>(stmt)) {
        if (current->scopeDepth > 0) current->addLocal(funDecl->name);

        Compiler compiler(current, FunctionType::TYPE_FUNCTION, funDecl->name);
        current = &compiler;
        
        current->beginScope();
        for (const auto& param : funDecl->parameters) {
            current->addLocal(param.first);
        }
        current->function->arity = funDecl->parameters.size();
        
        for (auto bStmt : funDecl->body->statements) {
            compileStatement(bStmt);
        }
        
        ObjFunction* function = current->endCompiler();
        current = current->enclosing;
        
        int constant = current->currentChunk()->addConstant(OBJ_VAL(function));
        current->emitBytes(static_cast<uint8_t>(OpCode::OP_CLOSURE), constant);
        for (int i = 0; i < function->upvalueCount; i++) {
            current->emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
            current->emitByte(compiler.upvalues[i].index);
        }
        
        if (current->scopeDepth == 0) {
            ObjString* nameStr = allocateString(funDecl->name);
            int nameConstant = current->currentChunk()->addConstant(OBJ_VAL(nameStr));
            current->emitBytes(static_cast<uint8_t>(OpCode::OP_DEFINE_GLOBAL), nameConstant);
        }
    }
    else if (auto retStmt = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
        if (current->type == FunctionType::TYPE_SCRIPT) {
            throw std::runtime_error("Can't return from top-level code.");
        }
        if (retStmt->argument) {
            compileExpression(retStmt->argument);
        } else {
            current->emitByte(static_cast<uint8_t>(OpCode::OP_NIL));
        }
        current->emitByte(static_cast<uint8_t>(OpCode::OP_RETURN));
    }
    else if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
        compileExpression(ifStmt->condition);
        int thenJump = current->emitJump(static_cast<uint8_t>(OpCode::OP_JUMP_IF_FALSE));
        current->emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        
        compileStatement(ifStmt->consequent);
        int elseJump = current->emitJump(static_cast<uint8_t>(OpCode::OP_JUMP));
        
        current->patchJump(thenJump);
        current->emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        
        if (ifStmt->alternate) compileStatement(ifStmt->alternate);
        current->patchJump(elseJump);
    }
    else if (auto whileStmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
        int loopStart = current->currentChunk()->code.size();
        compileExpression(whileStmt->condition);
        int exitJump = current->emitJump(static_cast<uint8_t>(OpCode::OP_JUMP_IF_FALSE));
        current->emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        
        compileStatement(whileStmt->body);
        
        current->emitLoop(loopStart);
        current->patchJump(exitJump);
        current->emitByte(static_cast<uint8_t>(OpCode::OP_POP));
    }
    else if (auto forStmt = std::dynamic_pointer_cast<ForStatement>(stmt)) {
        current->beginScope();
        if (forStmt->init) compileStatement(forStmt->init);
        
        int loopStart = current->currentChunk()->code.size();
        int exitJump = -1;
        
        if (forStmt->condition) {
            compileExpression(forStmt->condition);
            exitJump = current->emitJump(static_cast<uint8_t>(OpCode::OP_JUMP_IF_FALSE));
            current->emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        }
        
        compileStatement(forStmt->body);
        
        if (forStmt->update) {
            compileExpression(forStmt->update);
            current->emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        }
        
        current->emitLoop(loopStart);
        if (exitJump != -1) {
            current->patchJump(exitJump);
            current->emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        }
        current->endScope();
    }
    else if (auto blockStmt = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
        current->beginScope();
        for (auto bStmt : blockStmt->statements) compileStatement(bStmt);
        current->endScope();
    }
}

void AstCompiler::compileExpression(std::shared_ptr<Expression> expr) {
    if (auto num = std::dynamic_pointer_cast<NumberLiteral>(expr)) {
        current->emitConstant(NUMBER_VAL(num->value));
    }
    else if (auto str = std::dynamic_pointer_cast<StringLiteral>(expr)) {
        ObjString* objStr = allocateString(str->value);
        current->emitConstant(OBJ_VAL(objStr));
    }
    else if (auto bl = std::dynamic_pointer_cast<BooleanLiteral>(expr)) {
        current->emitByte(static_cast<uint8_t>(bl->value ? OpCode::OP_TRUE : OpCode::OP_FALSE));
    }
    else if (auto bin = std::dynamic_pointer_cast<BinaryExpression>(expr)) {
        if (bin->op == TokenType::LOGICAL_AND || bin->op == TokenType::LOGICAL_OR) {
            compileExpression(bin->left);
            if (bin->op == TokenType::LOGICAL_AND) {
                int endJump = current->emitJump(static_cast<uint8_t>(OpCode::OP_JUMP_IF_FALSE));
                current->emitByte(static_cast<uint8_t>(OpCode::OP_POP));
                compileExpression(bin->right);
                current->patchJump(endJump);
            } else {
                int elseJump = current->emitJump(static_cast<uint8_t>(OpCode::OP_JUMP_IF_FALSE));
                int endJump = current->emitJump(static_cast<uint8_t>(OpCode::OP_JUMP));
                current->patchJump(elseJump);
                current->emitByte(static_cast<uint8_t>(OpCode::OP_POP));
                compileExpression(bin->right);
                current->patchJump(endJump);
            }
            return;
        }

        compileExpression(bin->left);
        compileExpression(bin->right);
        switch (bin->op) {
            case TokenType::PLUS: current->emitByte(static_cast<uint8_t>(OpCode::OP_ADD)); break;
            case TokenType::MINUS: current->emitByte(static_cast<uint8_t>(OpCode::OP_SUBTRACT)); break;
            case TokenType::MULTIPLY: current->emitByte(static_cast<uint8_t>(OpCode::OP_MULTIPLY)); break;
            case TokenType::DIVIDE: current->emitByte(static_cast<uint8_t>(OpCode::OP_DIVIDE)); break;
            case TokenType::MODULO: current->emitByte(static_cast<uint8_t>(OpCode::OP_MODULO)); break;
            case TokenType::STRICT_EQUAL:
            case TokenType::EQUAL: current->emitByte(static_cast<uint8_t>(OpCode::OP_EQUAL)); break;
            case TokenType::LESS: current->emitByte(static_cast<uint8_t>(OpCode::OP_LESS)); break;
            case TokenType::LESS_EQUAL: current->emitByte(static_cast<uint8_t>(OpCode::OP_LESS_EQUAL)); break;
            case TokenType::GREATER: current->emitByte(static_cast<uint8_t>(OpCode::OP_GREATER)); break;
            case TokenType::GREATER_EQUAL: current->emitByte(static_cast<uint8_t>(OpCode::OP_GREATER_EQUAL)); break;
            default: throw std::runtime_error("Unsupported binary operator in compiler");
        }
    }
    else if (auto un = std::dynamic_pointer_cast<UnaryExpression>(expr)) {
        compileExpression(un->argument);
        switch (un->op) {
            case TokenType::MINUS: current->emitByte(static_cast<uint8_t>(OpCode::OP_NEGATE)); break;
            case TokenType::LOGICAL_NOT: current->emitByte(static_cast<uint8_t>(OpCode::OP_NOT)); break;
            default: throw std::runtime_error("Unsupported unary operator");
        }
    }
    else if (auto id = std::dynamic_pointer_cast<Identifier>(expr)) {
        int arg = current->resolveLocal(id->name);
        if (arg != -1) {
            current->emitBytes(static_cast<uint8_t>(OpCode::OP_GET_LOCAL), (uint8_t)arg);
        } else if ((arg = current->resolveUpvalue(id->name)) != -1) {
            current->emitBytes(static_cast<uint8_t>(OpCode::OP_GET_UPVALUE), (uint8_t)arg);
        } else {
            ObjString* nameStr = allocateString(id->name);
            int constant = current->currentChunk()->addConstant(OBJ_VAL(nameStr)); 
            current->emitBytes(static_cast<uint8_t>(OpCode::OP_GET_GLOBAL), constant);
        }
    }
    else if (auto assign = std::dynamic_pointer_cast<AssignmentExpression>(expr)) {
        if (auto id = std::dynamic_pointer_cast<Identifier>(assign->left)) {
            compileExpression(assign->value);
            int arg = current->resolveLocal(id->name);
            if (arg != -1) {
                current->emitBytes(static_cast<uint8_t>(OpCode::OP_SET_LOCAL), (uint8_t)arg);
            } else if ((arg = current->resolveUpvalue(id->name)) != -1) {
                current->emitBytes(static_cast<uint8_t>(OpCode::OP_SET_UPVALUE), (uint8_t)arg);
            } else {
                ObjString* nameStr = allocateString(id->name);
                int constant = current->currentChunk()->addConstant(OBJ_VAL(nameStr)); 
                current->emitBytes(static_cast<uint8_t>(OpCode::OP_SET_GLOBAL), constant);
            }
        }
        else if (auto mem = std::dynamic_pointer_cast<MemberExpression>(assign->left)) {
            compileExpression(mem->object);
            if (mem->computed) {
                compileExpression(mem->property);
                compileExpression(assign->value);
                current->emitByte(static_cast<uint8_t>(OpCode::OP_ARRAY_SET));
            } else {
                if (auto propId = std::dynamic_pointer_cast<Identifier>(mem->property)) {
                    compileExpression(assign->value);
                    ObjString* nameStr = allocateString(propId->name);
                    int constant = current->currentChunk()->addConstant(OBJ_VAL(nameStr));
                    current->emitBytes(static_cast<uint8_t>(OpCode::OP_SET_PROPERTY), constant);
                }
            }
        }
    }
    else if (auto call = std::dynamic_pointer_cast<CallExpression>(expr)) {
        compileExpression(call->callee);
        for (auto arg : call->arguments) {
            compileExpression(arg);
        }
        current->emitBytes(static_cast<uint8_t>(OpCode::OP_CALL), call->arguments.size());
    }
    else if (auto arr = std::dynamic_pointer_cast<ArrayLiteral>(expr)) {
        for (auto elem : arr->elements) {
            compileExpression(elem);
        }
        current->emitBytes(static_cast<uint8_t>(OpCode::OP_BUILD_ARRAY), arr->elements.size());
    }
    else if (auto obj = std::dynamic_pointer_cast<ObjectLiteral>(expr)) {
        for (auto prop : obj->properties) {
            ObjString* keyStr = allocateString(prop.key);
            int constant = current->currentChunk()->addConstant(OBJ_VAL(keyStr));
            current->emitBytes(static_cast<uint8_t>(OpCode::OP_CONSTANT), constant);
            compileExpression(prop.value);
        }
        current->emitBytes(static_cast<uint8_t>(OpCode::OP_BUILD_OBJECT), obj->properties.size());
    }
    else if (auto mem = std::dynamic_pointer_cast<MemberExpression>(expr)) {
        compileExpression(mem->object);
        if (mem->computed) {
            compileExpression(mem->property);
            current->emitByte(static_cast<uint8_t>(OpCode::OP_ARRAY_GET));
        } else {
            if (auto str = std::dynamic_pointer_cast<StringLiteral>(mem->property)) {
                ObjString* nameStr = allocateString(str->value);
                int constant = current->currentChunk()->addConstant(OBJ_VAL(nameStr));
                current->emitBytes(static_cast<uint8_t>(OpCode::OP_GET_PROPERTY), constant);
            } else if (auto id = std::dynamic_pointer_cast<Identifier>(mem->property)) {
                ObjString* nameStr = allocateString(id->name);
                int constant = current->currentChunk()->addConstant(OBJ_VAL(nameStr));
                current->emitBytes(static_cast<uint8_t>(OpCode::OP_GET_PROPERTY), constant);
            }
        }
    }
    else if (auto funcExpr = std::dynamic_pointer_cast<FunctionExpression>(expr)) {
        Compiler compiler(current, FunctionType::TYPE_FUNCTION, funcExpr->name);
        current = &compiler;
        
        current->beginScope();
        for (const auto& param : funcExpr->parameters) {
            current->addLocal(param.first);
        }
        current->function->arity = funcExpr->parameters.size();
        
        for (auto bStmt : funcExpr->body->statements) {
            compileStatement(bStmt);
        }
        
        ObjFunction* function = current->endCompiler();
        current = current->enclosing;
        
        int constant = current->currentChunk()->addConstant(OBJ_VAL(function));
        current->emitBytes(static_cast<uint8_t>(OpCode::OP_CLOSURE), constant);
        for (int i = 0; i < function->upvalueCount; i++) {
            current->emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
            current->emitByte(compiler.upvalues[i].index);
        }
    }
}
