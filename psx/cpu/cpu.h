#pragma once
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <utility>
#include <memory>
#include <memory/interconnect.h>
using std::unique_ptr;
using std::pair;

struct RegIndex {
	RegIndex(uint32_t i) : index(i) {}
	uint32_t index;
};

class Instruction {
public:
	Instruction(uint32_t op) { opcode = op; }
	
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
	uint32_t opcode;
};

enum class ExceptionType {
	ReadAddressError = 0x4,
	WriteAddressError = 0x5,
	SysCall = 0x8,
	Overflow = 0xc,
	Break = 0x9,
	CopError = 0xb,
	IllegalInstr = 0xa,
};

union Address {
	uint32_t raw;
	
	struct {
		uint32_t word_alignment : 2;
		uint32_t index : 2;
		uint32_t cache_line : 8;
		uint32_t tag : 19;
	};
};

class Interconnect;
class Renderer;
class CPU {
public:
	CPU(Renderer* renderer);
	~CPU() = default;

	void tick();
	void reset();
	void execute(Instruction instr);

	template <typename T = uint32_t>
	T read(uint32_t addr);

	template <typename T = uint32_t>
	void write(uint32_t addr, T data);

	uint32_t get_reg(RegIndex reg);
	void set_reg(RegIndex reg, uint32_t val);
	
	void branch(uint32_t offset);
	void exception(ExceptionType type);
	bool cache_write();

	void op_lui(Instruction instr);
	void op_ori(Instruction instr);
	void op_sw(Instruction instr);
	void op_lw(Instruction instr);
	void op_sll(Instruction instr);
	void op_addiu(Instruction instr);
	void op_addi(Instruction instr);
	void op_j(Instruction instr);
	void op_or(Instruction instr);
	void op_cop0(Instruction instr);
	void op_mfc0(Instruction instr);
	void op_mtc0(Instruction instr);
	void op_cop1(Instruction instr);
	void op_cop2(Instruction instr);
	void op_cop3(Instruction instr);
	void op_bne(Instruction instr);
	void op_sltu(Instruction instr);
	void op_addu(Instruction instr);
	void op_sh(Instruction instr);
	void op_jal(Instruction instr);
	void op_andi(Instruction instr);
	void op_sb(Instruction instr);
	void op_jr(Instruction instr);
	void op_lb(Instruction instr);
	void op_beq(Instruction instr);
	void op_and(Instruction instr);
	void op_add(Instruction instr);
	void op_bgtz(Instruction instr);
	void op_blez(Instruction instr);
	void op_lbu(Instruction instr);
	void op_jalr(Instruction instr);
	void op_bxx(Instruction instr);
	void op_slti(Instruction instr);
	void op_subu(Instruction instr);
	void op_sra(Instruction instr);
	void op_div(Instruction instr);
	void op_mlfo(Instruction instr);
	void op_nor(Instruction instr);
	void op_slt(Instruction instr);
	void op_sltiu(Instruction instr);
	void op_sub(Instruction instr);
	void op_xor(Instruction instr);
	void op_xori(Instruction instr);
	void op_sllv(Instruction instr);
	void op_srav(Instruction instr);
	void op_srl(Instruction instr);
	void op_srlv(Instruction instr);
	void op_divu(Instruction instr);
	void op_mfhi(Instruction instr);
	void op_mflo(Instruction instr);
	void op_mthi(Instruction instr);
	void op_mtlo(Instruction instr);
	void op_mult(Instruction instr);
	void op_multu(Instruction instr);
	void op_break(Instruction instr);
	void op_syscall(Instruction instr);
	void op_rfe(Instruction instr);
	void op_lhu(Instruction instr);
	void op_lh(Instruction instr);
	void op_lwl(Instruction instr);
	void op_lwr(Instruction instr);
	void op_swl(Instruction instr);
	void op_swr(Instruction instr);
	void op_lwc0(Instruction instr);
	void op_lwc1(Instruction instr);
	void op_lwc2(Instruction instr);
	void op_lwc3(Instruction instr);
	void op_swc0(Instruction instr);
	void op_swc1(Instruction instr);
	void op_swc2(Instruction instr);
	void op_swc3(Instruction instr);
	void op_illegal(Instruction instr);

public:
	uint32_t pc, current_pc, next_pc, hi, low;
	uint32_t regs[32], out_regs[32];
	pair<RegIndex, uint32_t> load;

	/* Coprocessor 0 */
	uint32_t sr, cause, epc; // Reg $12

	unique_ptr<Interconnect> inter;
	Renderer* renderer;

	bool isbranch, isdelay_slot;
};

template<typename T>
inline T CPU::read(uint32_t addr)
{
	return inter->read<T>(addr);
}

template<typename T>
inline void CPU::write(uint32_t addr, T data)
{
	if ((sr & 0x10000) != 0) {
		std::clog << "Ignore write to ram while cache is enabled!\n";
		return;
	}

	inter->write<T>(addr, data);
}
