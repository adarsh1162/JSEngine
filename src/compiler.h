#pragma once
#include "ast.h"
#include "chunk.h"
#include "gc.h"
#include <unordered_map>
#include <string>
#include <memory>

enum class FunctionType {
    TYPE_SCRIPT,
    TYPE_FUNCTION
};

struct Local {
    std::string name;
    int depth;
    bool isCaptured;
};

struct Upvalue {
    uint8_t index;
    bool isLocal;
};

class Compiler {
public:
    Compiler* enclosing;
    ObjFunction* function;
    FunctionType type;

    Local locals[256];
    int localCount;
    Upvalue upvalues[256];
    int scopeDepth;

    Compiler(Compiler* enclosing, FunctionType type, const std::string& name);
    ObjFunction* endCompiler();

    Chunk* currentChunk();

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
    int resolveUpvalue(const std::string& name);
    int addUpvalue(uint8_t index, bool isLocal);
};

class AstCompiler {
public:
    Compiler* current;

    AstCompiler();
    ObjFunction* compile(std::shared_ptr<Program> program);

private:
    void compileStatement(std::shared_ptr<Statement> stmt);
    void compileExpression(std::shared_ptr<Expression> expr);
};
