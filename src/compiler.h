#pragma once
#include "ast.h"
#include "chunk.h"
#include <unordered_map>
#include <string>
#include <memory>

struct Local {
    std::string name;
    int depth;
};

class Compiler {
public:
    Chunk* compilingChunk;
    Local locals[256];
    int localCount;
    int scopeDepth;

    Compiler();
    bool compile(std::shared_ptr<Program> program, Chunk* chunk);

private:
    void emitByte(uint8_t byte);
    void emitBytes(uint8_t byte1, uint8_t byte2);
    void emitConstant(Value value);
    void emitReturn();
    int emitJump(uint8_t instruction);
    void patchJump(int offset);
    void emitLoop(int loopStart);

    void beginScope();
    void endScope();

    void addLocal(const std::string& name);
    int resolveLocal(const std::string& name);

    void compileStatement(std::shared_ptr<Statement> stmt);
    void compileExpression(std::shared_ptr<Expression> expr);
};
