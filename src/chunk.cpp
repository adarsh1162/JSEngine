#include "chunk.h"
#include <iostream>
#include <iomanip>

void Chunk::write(uint8_t byte, int line) {
    code.push_back(byte);
    lines.push_back(line);
}

int Chunk::addConstant(Value value) {
    constants.push_back(value);
    return constants.size() - 1;
}

static int simpleInstruction(const char* name, int offset) {
    std::cout << name << "\n";
    return offset + 1;
}

static int byteInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    std::cout << std::left << std::setw(16) << name << " " << std::setw(4) << (int)slot << "\n";
    return offset + 2;
}

static int jumpInstruction(const char* name, int sign, Chunk* chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    std::cout << std::left << std::setw(16) << name << " " << offset << " -> " 
              << offset + 3 + sign * jump << "\n";
    return offset + 3;
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    std::cout << std::left << std::setw(16) << name << " " 
              << std::setw(4) << (int)constant << " '";
    printValue(chunk->constants[constant]);
    std::cout << "'\n";
    return offset + 2;
}

void Chunk::disassemble(const std::string& name) {
    std::cout << "== " << name << " ==\n";
    for (int offset = 0; offset < code.size();) {
        offset = disassembleInstruction(offset);
    }
}

int Chunk::disassembleInstruction(int offset) {
    std::cout << std::setfill('0') << std::setw(4) << offset << " " << std::setfill(' ');
    
    if (offset > 0 && lines[offset] == lines[offset - 1]) {
        std::cout << "   | ";
    } else {
        std::cout << std::setw(4) << lines[offset] << " ";
    }

    uint8_t instruction = code[offset];
    switch (static_cast<OpCode>(instruction)) {
        case OpCode::OP_CONSTANT: return constantInstruction("OP_CONSTANT", this, offset);
        case OpCode::OP_NIL: return simpleInstruction("OP_NIL", offset);
        case OpCode::OP_TRUE: return simpleInstruction("OP_TRUE", offset);
        case OpCode::OP_FALSE: return simpleInstruction("OP_FALSE", offset);
        case OpCode::OP_POP: return simpleInstruction("OP_POP", offset);
        case OpCode::OP_GET_LOCAL: return byteInstruction("OP_GET_LOCAL", this, offset);
        case OpCode::OP_SET_LOCAL: return byteInstruction("OP_SET_LOCAL", this, offset);
        case OpCode::OP_GET_GLOBAL: return constantInstruction("OP_GET_GLOBAL", this, offset);
        case OpCode::OP_DEFINE_GLOBAL: return constantInstruction("OP_DEFINE_GLOBAL", this, offset);
        case OpCode::OP_SET_GLOBAL: return constantInstruction("OP_SET_GLOBAL", this, offset);
        case OpCode::OP_EQUAL: return simpleInstruction("OP_EQUAL", offset);
        case OpCode::OP_GREATER: return simpleInstruction("OP_GREATER", offset);
        case OpCode::OP_LESS: return simpleInstruction("OP_LESS", offset);
        case OpCode::OP_ADD: return simpleInstruction("OP_ADD", offset);
        case OpCode::OP_SUBTRACT: return simpleInstruction("OP_SUBTRACT", offset);
        case OpCode::OP_MULTIPLY: return simpleInstruction("OP_MULTIPLY", offset);
        case OpCode::OP_DIVIDE: return simpleInstruction("OP_DIVIDE", offset);
        case OpCode::OP_MODULO: return simpleInstruction("OP_MODULO", offset);
        case OpCode::OP_POWER: return simpleInstruction("OP_POWER", offset);
        case OpCode::OP_NOT: return simpleInstruction("OP_NOT", offset);
        case OpCode::OP_NEGATE: return simpleInstruction("OP_NEGATE", offset);
        case OpCode::OP_PRINT: return simpleInstruction("OP_PRINT", offset);
        case OpCode::OP_JUMP: return jumpInstruction("OP_JUMP", 1, this, offset);
        case OpCode::OP_JUMP_IF_FALSE: return jumpInstruction("OP_JUMP_IF_FALSE", 1, this, offset);
        case OpCode::OP_LOOP: return jumpInstruction("OP_LOOP", -1, this, offset);
        case OpCode::OP_RETURN: return simpleInstruction("OP_RETURN", offset);
        default:
            std::cout << "Unknown opcode " << (int)instruction << "\n";
            return offset + 1;
    }
}
