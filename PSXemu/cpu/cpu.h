#pragma once
#include <cstdio>
#include <cstdint>
#include <utility>
#include <unordered_map>

#include <cpu/instr.h>
#include <cpu/cop0.h>
#include <memory/bus.h>

#define CPU_FREQ 33.8685

typedef std::function<void()> CPUinstr;
typedef std::pair<uint32_t, uint32_t> LoadDelay;
typedef std::unordered_map<uint32_t, CPUinstr> LookupTable;

/* Memory Ranges. */
const Range KSEG = Range(0x00000000, 2048 * 1024 * 1024);
const Range KSEG0 = Range(0x80000000, 512 * 1024 * 1024);
const Range KSEG1 = Range(0xA0000000, 512 * 1024 * 1024);
const Range KSEG2 = Range(0xC0000000, 1024 * 1024 * 1024);

/* A class implemeting the MIPS CPU. */
class Renderer;
class CPU {
public:
	CPU(Renderer* renderer, Bus* _bus);
	~CPU() = default;

	/* Flow control functions. */
	void tick();
	void fetch();
	void reset();
	void branch(uint32_t offset);
	void exception(ExceptionType type);
	void register_opcodes();

	void set_ext_irq(bool val);
	void trigger_irq(Interrupt irq);

	/* Bus interface functions. */
	template <typename T = uint32_t>
	T read(uint32_t addr);

	template <typename T = uint32_t>
	void write(uint32_t addr, T data);

	/* Register read/write. */
	uint32_t get_reg(uint32_t reg);
	void set_reg(uint32_t reg, uint32_t val);
	
	/* MIPS R3000A Opcodes. */
	/* Select the right sub-opcode. */
	void op_special();
	
	/* Memory read instructions. */
	void op_lb(); void op_lh(); void op_lw(); void op_lbu(); 
	void op_lhu(); void op_lui(); void op_lwl(); void op_lwr();

	/* Memory write instructions. */
	void op_sb(); void op_sh(); void op_sw();
	void op_swl(); void op_swr();

	/* Branch instruction. */
	void op_j(); void op_jr(); void op_jal(); void op_jalr();
	void op_bne(); void op_beq(); void op_bgtz(); void op_blez();
	void op_bxx();
	
	/* Bitwise manipulation instuctions. */
	void op_and(); void op_andi(); void op_or(); void op_ori();
	void op_nor(); void op_xor(); void op_xori();
	
	/* Arithmetic shift rotate instructions. */
	void op_slt(); void op_sltu(); void op_slti(); void op_sltiu();
	void op_sra(); void op_srav(); void op_srl(); void op_srlv();
	void op_sll(); void op_sllv();

	/* Coprocessor move/store instuctions. */
	void op_cop0(); void op_cop1(); void op_cop2(); void op_cop3();
	void op_mfhi(); void op_mflo(); void op_mthi(); void op_mtlo();
	void op_mlfo(); void op_mfc0(); void op_mtc0();

	/* Arithmetic instuctions. */
	void op_add(); void op_addu(); void op_addi(); void op_addiu();
	void op_sub(); void op_subu();
	void op_div(); void op_divu(); void op_mult(); void op_multu();
	
	void op_break(); void op_syscall(); void op_rfe();
	
	/* Coprocessor instuctions. */
	void op_lwc0(); void op_lwc1(); void op_lwc2(); void op_lwc3();
	void op_swc0(); void op_swc1(); void op_swc2(); void op_swc3();
	
	/* Illegal instruction end up here. */
	void op_illegal();

public:
	/* PC and HI/LO registers. */
	uint32_t pc, current_pc, next_pc;
	uint32_t hi, lo;
	
	/* General purpose registers. */
	uint32_t regs[32];
	uint32_t out_regs[32];
	
	/* Coprocessor 0. */
	Cop0 cop0;

	/* Instruction lookup tables. */
	LookupTable lookup, special;
	
	Instruction instr;
	LoadDelay load;
	
	/* Instruction cache. */
	CacheLine instr_cache[256] = {};

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
		CacheControl& cc = bus->cache_ctrl;
		
		Address address;
		address.raw = addr;

		/* Check if caching is enabled. */
		if (!cc.is1) {
			printf("Unsupported write while cache is enabled!\n");
			exit(0);
		}

		CacheLine& line = instr_cache[address.cache_line];

		/* Invalid cache line if TAG test is enabled. */
		if (cc.tag) {
			/* Invalidate by pushing index out of range. */
			line.tag.index = 4;
		} /* Write to cache. */
		else {
			line.instrs[address.index].raw = data;
		}
	}
	else {
		bus->write<T>(addr, data);
	}
}
