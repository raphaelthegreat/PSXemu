#pragma once
#include <cstdint>

struct RegIndex {
	RegIndex(uint32_t i) : index(i) {}
	uint32_t index;
};

class Instruction {
public:
	Instruction() = default;
	Instruction(uint32_t op) : opcode(op) {}

	uint32_t type() { return opcode >> 26; }
	uint32_t subtype() { return opcode & 0x3f; }
	RegIndex rt() { return ((opcode >> 16) & 0x1f); }
	RegIndex rs() { return ((opcode >> 21) & 0x1f); }
	RegIndex rd() { return ((opcode >> 11)) & 0x1f; }
	uint32_t imm() { return (opcode & 0xffff); }
	uint32_t simm() { int16_t val = opcode & 0xffff; return val; }
	uint32_t imm_jump() { return (opcode & 0x3ffffff); }
	uint32_t cop_op() { return ((opcode >> 21) & 0x1f); }
	uint32_t shift() { return (opcode >> 6) & 0x1f; }

public:
	uint32_t opcode = 0;
};