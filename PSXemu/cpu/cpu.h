#pragma once
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <utility>
#include <memory>
#include <cpu/instr.h>
#include <cpu/cop0.h>
#include <memory/bus.h>
using std::unique_ptr;
using std::pair;

enum ExceptionType {
	ReadError = 0x4,
	WriteError = 0x5,
	SysCall = 0x8,
	Overflow = 0xc,
	Break = 0x9,
	CopError = 0xb,
	IllegalInstr = 0xa,
};

class Renderer;
class CPU {
public:
	CPU(Renderer* renderer);
	~CPU();

	void tick();
	void reset();
	void execute();

	template <typename T = uint32_t>
	T read(uint32_t addr);

	template <typename T = uint32_t>
	void write(uint32_t addr, T data);

	uint32_t get_reg(RegIndex reg);
	void set_reg(RegIndex reg, uint32_t val);
	
	void branch(uint32_t offset);
	void exception(ExceptionType type);

	void op_lui();
	void op_ori();
	void op_sw();
	void op_lw();
	void op_sll();
	void op_addiu();
	void op_addi();
	void op_j();
	void op_or();
	void op_cop0();
	void op_mfc0();
	void op_mtc0();
	void op_cop1();
	void op_cop2();
	void op_cop3();
	void op_bne();
	void op_sltu();
	void op_addu();
	void op_sh();
	void op_jal();
	void op_andi();
	void op_sb();
	void op_jr();
	void op_lb();
	void op_beq();
	void op_and();
	void op_add();
	void op_bgtz();
	void op_blez();
	void op_lbu();
	void op_jalr();
	void op_bxx();
	void op_slti();
	void op_subu();
	void op_sra();
	void op_div();
	void op_mlfo();
	void op_nor();
	void op_slt();
	void op_sltiu();
	void op_sub();
	void op_xor();
	void op_xori();
	void op_sllv();
	void op_srav();
	void op_srl();
	void op_srlv();
	void op_divu();
	void op_mfhi();
	void op_mflo();
	void op_mthi();
	void op_mtlo();
	void op_mult();
	void op_multu();
	void op_break();
	void op_syscall();
	void op_rfe();
	void op_lhu();
	void op_lh();
	void op_lwl();
	void op_lwr();
	void op_swl();
	void op_swr();
	void op_lwc0();
	void op_lwc1();
	void op_lwc2();
	void op_lwc3();
	void op_swc0();
	void op_swc1();
	void op_swc2();
	void op_swc3();
	void op_illegal();

public:
	uint32_t pc, current_pc, next_pc;
	uint32_t hi, low;
	
	uint32_t regs[32];
	uint32_t out_regs[32];
	
	/* Coprocessor 0 */
	Cop0 cop0;
	Instruction instr;

	pair<RegIndex, uint32_t> load;
	
	Bus* bus;
	Renderer* renderer;

	bool should_branch, delay_slot;
};

template<typename T>
inline T CPU::read(uint32_t addr)
{
	return bus->read<T>(addr);
}

template<typename T>
inline void CPU::write(uint32_t addr, T data)
{
	if (cop0.sr.IsC) {
		std::clog << "Ignore write to ram while cache is enabled!\n";
		return;
	}

	bus->write<T>(addr, data);
}
