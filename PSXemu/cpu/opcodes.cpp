#include "cpu.h"
#include <functional>

#define BIND_CPU(x) std::bind(&CPU::x, this)

void CPU::op_special()
{
	auto& handler = special[instr.r_type.funct];
	handler();
}

void CPU::register_opcodes()
{
	lookup[0b000000] = BIND_CPU(op_special);
	lookup[0b000001] = BIND_CPU(op_bxx);
	lookup[0b001111] = BIND_CPU(op_lui);
	lookup[0b001101] = BIND_CPU(op_ori);
	lookup[0b101011] = BIND_CPU(op_sw);
	lookup[0b001001] = BIND_CPU(op_addiu);
	lookup[0b001000] = BIND_CPU(op_addi);
	lookup[0b000010] = BIND_CPU(op_j);
	lookup[0b010000] = BIND_CPU(op_cop0);
	lookup[0b010001] = BIND_CPU(op_cop1);
	lookup[0b010010] = BIND_CPU(op_cop2);
	lookup[0b010011] = BIND_CPU(op_cop3);
	lookup[0b100011] = BIND_CPU(op_lw);
	lookup[0b000101] = BIND_CPU(op_bne);
	lookup[0b101001] = BIND_CPU(op_sh);
	lookup[0b000011] = BIND_CPU(op_jal);
	lookup[0b001100] = BIND_CPU(op_andi);
	lookup[0b101000] = BIND_CPU(op_sb);
	lookup[0b100000] = BIND_CPU(op_lb);
	lookup[0b000100] = BIND_CPU(op_beq);
	lookup[0b000111] = BIND_CPU(op_bgtz);
	lookup[0b000110] = BIND_CPU(op_blez);
	lookup[0b100100] = BIND_CPU(op_lbu);
	lookup[0b001010] = BIND_CPU(op_slti);
	lookup[0b001011] = BIND_CPU(op_sltiu);
	lookup[0b100101] = BIND_CPU(op_lhu);
	lookup[0b100001] = BIND_CPU(op_lh);
	lookup[0b001110] = BIND_CPU(op_xori);
	lookup[0b110000] = BIND_CPU(op_lwc0);
	lookup[0b110001] = BIND_CPU(op_lwc1);
	lookup[0b110010] = BIND_CPU(op_lwc2);
	lookup[0b110011] = BIND_CPU(op_lwc3);
	lookup[0b111000] = BIND_CPU(op_swc0);
	lookup[0b111001] = BIND_CPU(op_swc1);
	lookup[0b111010] = BIND_CPU(op_swc2);
	lookup[0b111011] = BIND_CPU(op_swc3);
	lookup[0b101110] = BIND_CPU(op_swr);
	lookup[0b101010] = BIND_CPU(op_swl);
	lookup[0b100010] = BIND_CPU(op_lwl);
	lookup[0b100110] = BIND_CPU(op_lwr);
	
	special[0b000000] = BIND_CPU(op_sll);
	special[0b100101] = BIND_CPU(op_or);
	special[0b101011] = BIND_CPU(op_sltu);
	special[0b100001] = BIND_CPU(op_addu);
	special[0b001000] = BIND_CPU(op_jr);
	special[0b100100] = BIND_CPU(op_and);
	special[0b100000] = BIND_CPU(op_add);
	special[0b001001] = BIND_CPU(op_jalr);
	special[0b100011] = BIND_CPU(op_subu);
	special[0b000011] = BIND_CPU(op_sra);
	special[0b011010] = BIND_CPU(op_div);
	special[0b010010] = BIND_CPU(op_mlfo);
	special[0b000010] = BIND_CPU(op_srl);
	special[0b011011] = BIND_CPU(op_divu);
	special[0b010000] = BIND_CPU(op_mfhi);
	special[0b101010] = BIND_CPU(op_slt);
	special[0b001100] = BIND_CPU(op_syscall);
	special[0b010011] = BIND_CPU(op_mtlo);
	special[0b010001] = BIND_CPU(op_mthi);
	special[0b000100] = BIND_CPU(op_sllv);
	special[0b100111] = BIND_CPU(op_nor);
	special[0b000111] = BIND_CPU(op_srav);
	special[0b000110] = BIND_CPU(op_srlv);
	special[0b011001] = BIND_CPU(op_multu);
	special[0b100110] = BIND_CPU(op_xor);
	special[0b001101] = BIND_CPU(op_break);
	special[0b011000] = BIND_CPU(op_mult);
	special[0b100010] = BIND_CPU(op_sub);
}