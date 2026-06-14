#include "compiler.h"
#include "gc.h"
#include <iostream>

Compiler::Compiler() : compilingChunk(nullptr), localCount(0), scopeDepth(0) {}

bool Compiler::compile(std::shared_ptr<Program> program, Chunk* chunk) {
    compilingChunk = chunk;
    localCount = 0;
    scopeDepth = 0;
    
    // Add dummy local for global scope slot 0
    Local& local = locals[localCount++];
    local.depth = 0;
    local.name = "";

    try {
        for (auto stmt : program->body) {
            compileStatement(stmt);
        }
        emitReturn();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Compile error: " << e.what() << "\n";
        return false;
    }
}

void Compiler::emitByte(uint8_t byte) {
    compilingChunk->write(byte, 1); // Mock line number for now
}

void Compiler::emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

void Compiler::emitConstant(Value value) {
    int constant = compilingChunk->addConstant(value);
    if (constant > 255) {
        throw std::runtime_error("Too many constants in one chunk.");
    }
    emitBytes(static_cast<uint8_t>(OpCode::OP_CONSTANT), constant);
}

int Compiler::emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return compilingChunk->code.size() - 2;
}

void Compiler::patchJump(int offset) {
    int jump = compilingChunk->code.size() - offset - 2;
    if (jump > 65535) throw std::runtime_error("Too much code to jump over.");
    compilingChunk->code[offset] = (jump >> 8) & 0xff;
    compilingChunk->code[offset + 1] = jump & 0xff;
}

void Compiler::emitLoop(int loopStart) {
    emitByte(static_cast<uint8_t>(OpCode::OP_LOOP));
    int jump = compilingChunk->code.size() - loopStart + 2;
    if (jump > 65535) throw std::runtime_error("Loop body too large.");
    emitByte((jump >> 8) & 0xff);
    emitByte(jump & 0xff);
}

void Compiler::emitReturn() {
    emitByte(static_cast<uint8_t>(OpCode::OP_RETURN));
}

void Compiler::beginScope() {
    scopeDepth++;
}

void Compiler::endScope() {
    scopeDepth--;
    while (localCount > 0 && locals[localCount - 1].depth > scopeDepth) {
        emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        localCount--;
    }
}

void Compiler::addLocal(const std::string& name) {
    if (localCount == 256) throw std::runtime_error("Too many local variables in function.");
    Local& local = locals[localCount++];
    local.name = name;
    local.depth = scopeDepth;
}

int Compiler::resolveLocal(const std::string& name) {
    for (int i = localCount - 1; i >= 0; i--) {
        if (locals[i].name == name) {
            return i;
        }
    }
    return -1;
}

void Compiler::compileStatement(std::shared_ptr<Statement> stmt) {
    if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
        compileExpression(exprStmt->expression);
        emitByte(static_cast<uint8_t>(OpCode::OP_POP));
    }
    else if (auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(stmt)) {
        if (varDecl->initializer) {
            compileExpression(varDecl->initializer);
        } else {
            emitByte(static_cast<uint8_t>(OpCode::OP_NIL));
        }
        
        if (scopeDepth > 0) {
            addLocal(varDecl->name);
        } else {
            // Global variable
            ObjString* nameStr = allocateString(varDecl->name);
            int constant = compilingChunk->addConstant(OBJ_VAL(nameStr)); 
            emitBytes(static_cast<uint8_t>(OpCode::OP_DEFINE_GLOBAL), constant);
        }
    }
    else if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
        compileExpression(ifStmt->condition);
        int thenJump = emitJump(static_cast<uint8_t>(OpCode::OP_JUMP_IF_FALSE));
        emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        
        compileStatement(ifStmt->consequent);
        int elseJump = emitJump(static_cast<uint8_t>(OpCode::OP_JUMP));
        
        patchJump(thenJump);
        emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        
        if (ifStmt->alternate) compileStatement(ifStmt->alternate);
        patchJump(elseJump);
    }
    else if (auto whileStmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
        int loopStart = compilingChunk->code.size();
        compileExpression(whileStmt->condition);
        int exitJump = emitJump(static_cast<uint8_t>(OpCode::OP_JUMP_IF_FALSE));
        emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        
        compileStatement(whileStmt->body);
        
        emitLoop(loopStart);
        patchJump(exitJump);
        emitByte(static_cast<uint8_t>(OpCode::OP_POP));
    }
    else if (auto forStmt = std::dynamic_pointer_cast<ForStatement>(stmt)) {
        beginScope();
        if (forStmt->init) compileStatement(forStmt->init);
        
        int loopStart = compilingChunk->code.size();
        int exitJump = -1;
        
        if (forStmt->condition) {
            compileExpression(forStmt->condition);
            exitJump = emitJump(static_cast<uint8_t>(OpCode::OP_JUMP_IF_FALSE));
            emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        }
        
        compileStatement(forStmt->body);
        
        if (forStmt->update) {
            compileExpression(forStmt->update);
            emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        }
        
        emitLoop(loopStart);
        if (exitJump != -1) {
            patchJump(exitJump);
            emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        }
        endScope();
    }
    else if (auto blockStmt = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
        beginScope();
        for (auto bStmt : blockStmt->statements) compileStatement(bStmt);
        endScope();
    }
}

void Compiler::compileExpression(std::shared_ptr<Expression> expr) {
    if (auto num = std::dynamic_pointer_cast<NumberLiteral>(expr)) {
        emitConstant(NUMBER_VAL(num->value));
    }
    else if (auto str = std::dynamic_pointer_cast<StringLiteral>(expr)) {
        ObjString* objStr = allocateString(str->value);
        emitConstant(OBJ_VAL(objStr));
    }
    else if (auto bin = std::dynamic_pointer_cast<BinaryExpression>(expr)) {
        compileExpression(bin->left);
        compileExpression(bin->right);
        switch (bin->op) {
            case TokenType::PLUS: emitByte(static_cast<uint8_t>(OpCode::OP_ADD)); break;
            case TokenType::MINUS: emitByte(static_cast<uint8_t>(OpCode::OP_SUBTRACT)); break;
            case TokenType::MULTIPLY: emitByte(static_cast<uint8_t>(OpCode::OP_MULTIPLY)); break;
            case TokenType::DIVIDE: emitByte(static_cast<uint8_t>(OpCode::OP_DIVIDE)); break;
            case TokenType::MODULO: emitByte(static_cast<uint8_t>(OpCode::OP_MODULO)); break;
            case TokenType::STRICT_EQUAL:
            case TokenType::EQUAL: emitByte(static_cast<uint8_t>(OpCode::OP_EQUAL)); break;
            case TokenType::LESS: emitByte(static_cast<uint8_t>(OpCode::OP_LESS)); break;
            case TokenType::LESS_EQUAL: emitByte(static_cast<uint8_t>(OpCode::OP_LESS_EQUAL)); break;
            case TokenType::GREATER: emitByte(static_cast<uint8_t>(OpCode::OP_GREATER)); break;
            case TokenType::GREATER_EQUAL: emitByte(static_cast<uint8_t>(OpCode::OP_GREATER_EQUAL)); break;
            default: throw std::runtime_error("Unsupported binary operator in compiler");
        }
    }
    else if (auto id = std::dynamic_pointer_cast<Identifier>(expr)) {
        int arg = resolveLocal(id->name);
        if (arg != -1) {
            emitBytes(static_cast<uint8_t>(OpCode::OP_GET_LOCAL), (uint8_t)arg);
        } else {
            ObjString* nameStr = allocateString(id->name);
            int constant = compilingChunk->addConstant(OBJ_VAL(nameStr)); 
            emitBytes(static_cast<uint8_t>(OpCode::OP_GET_GLOBAL), constant);
        }
    }
    else if (auto assign = std::dynamic_pointer_cast<AssignmentExpression>(expr)) {
        compileExpression(assign->value);
        if (auto id = std::dynamic_pointer_cast<Identifier>(assign->left)) {
            int arg = resolveLocal(id->name);
            if (arg != -1) {
                emitBytes(static_cast<uint8_t>(OpCode::OP_SET_LOCAL), (uint8_t)arg);
            } else {
                ObjString* nameStr = allocateString(id->name);
                int constant = compilingChunk->addConstant(OBJ_VAL(nameStr)); 
                emitBytes(static_cast<uint8_t>(OpCode::OP_SET_GLOBAL), constant);
            }
        }
    }
    else if (auto call = std::dynamic_pointer_cast<CallExpression>(expr)) {
        if (auto mem = std::dynamic_pointer_cast<MemberExpression>(call->callee)) {
            if (auto objId = std::dynamic_pointer_cast<Identifier>(mem->object)) {
                if (objId->name == "console" && !mem->computed) {
                    if (auto propStr = std::dynamic_pointer_cast<StringLiteral>(mem->property)) {
                        if (propStr->value == "log") {
                            compileExpression(call->arguments[0]);
                            emitByte(static_cast<uint8_t>(OpCode::OP_PRINT));
                            return;
                        }
                    }
                }
            }
        }
    }
}
