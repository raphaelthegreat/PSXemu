#pragma once
#include <cstdint>

union IType {
	uint32_t raw;

	struct {
		uint32_t data : 16;
		uint32_t rt : 5;
		uint32_t rs : 5;
		uint32_t opcode : 6;
	};
};

union RType {
	uint32_t raw;

	struct {
		uint32_t funct : 6;
		uint32_t shamt : 5;
		uint32_t rd : 5;
		uint32_t rt : 5;
		uint32_t rs : 5;
		uint32_t opcode : 6;
	};
};

union JType {
	uint32_t raw;

	struct {
		uint32_t target : 26;
		uint32_t opcode : 6;
	};
};

union Instruction {
	uint32_t raw;

	IType i_type;
	JType j_type;
	RType r_type;
};