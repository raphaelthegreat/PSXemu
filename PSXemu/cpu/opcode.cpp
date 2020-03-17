#include "cpu.h"
#include <functional>

#define BIND_CPU(x) std::bind(&CPU::x, this)
#pragma optimize("", off)

void CPU::op_special()
{
	auto func = instr.function();

	auto& handler = special[func];
	handler();
}

void CPU::op_cop0()
{
	switch (instr.rs()) {
	case 0b00000: op_mfc0(); break;
	case 0b00100: op_mtc0(); break;
	case 0b10000: op_rfe(); break;
	default: exception(ExceptionType::IllegalInstr); break;
	}
}

void CPU::op_cop2()
{
	uint32_t rs = instr.rs();

	switch (rs & 0x10) {
	case 0x00: {
		switch (rs) {
		case 0b00000: { op_mfc2(); break; }
		case 0b00010: { op_cfc2(); break; }
		case 0b00100: { op_mtc2(); break; }
		case 0b00110: { op_ctc2(); break; }
		default: { printf("Ill\n"); exception(ExceptionType::IllegalInstr); break; }
		}
		break;
	}
	case 0x10: {
		gte.execute(instr);
		break;
	}
	}
}

void CPU::register_opcodes()
{
	lookup[0b000000] = BIND_CPU(op_special);
	lookup[0b000001] = BIND_CPU(op_bcond);
	lookup[0b001111] = BIND_CPU(op_lui);
	lookup[0b001101] = BIND_CPU(op_ori);
	lookup[0b101011] = BIND_CPU(op_sw);
	lookup[0b001001] = BIND_CPU(op_addiu);
	lookup[0b001000] = BIND_CPU(op_addi);
	lookup[0b000010] = BIND_CPU(op_j);
	lookup[0b010000] = BIND_CPU(op_cop0);
	lookup[0b010010] = BIND_CPU(op_cop2);
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
	lookup[0b101110] = BIND_CPU(op_swr);
	lookup[0b101010] = BIND_CPU(op_swl);
	lookup[0b100010] = BIND_CPU(op_lwl);
	lookup[0b100110] = BIND_CPU(op_lwr);
	lookup[0b110010] = BIND_CPU(op_lwc2);
	lookup[0b111010] = BIND_CPU(op_swc2);

	str_lookup[0b000001] = "bcond";
	str_lookup[0b001111] = "lui";
	str_lookup[0b001101] = "ori";
	str_lookup[0b101011] = "sw";
	str_lookup[0b001001] = "addiu";
	str_lookup[0b001000] = "addi";
	str_lookup[0b000010] = "j";
	str_lookup[0b010000] = "cop0";
	str_lookup[0b010010] = "cop2";
	str_lookup[0b100011] = "lw";
	str_lookup[0b000101] = "bne";
	str_lookup[0b101001] = "sh";
	str_lookup[0b000011] = "jal";
	str_lookup[0b001100] = "andi";
	str_lookup[0b101000] = "sb";
	str_lookup[0b100000] = "lb";
	str_lookup[0b000100] = "beq";
	str_lookup[0b000111] = "bgtz";
	str_lookup[0b000110] = "blez";
	str_lookup[0b100100] = "lbu";
	str_lookup[0b001010] = "slti";
	str_lookup[0b001011] = "sltiu";
	str_lookup[0b100101] = "lhu";
	str_lookup[0b100001] = "lh";
	str_lookup[0b001110] = "xori";
	str_lookup[0b101110] = "swr";
	str_lookup[0b101010] = "swl";
	str_lookup[0b100010] = "lwl";
	str_lookup[0b100110] = "lwr";
	str_lookup[0b110010] = "lwc2";
	str_lookup[0b111010] = "swc2";

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
	special[0b010010] = BIND_CPU(op_mflo);
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

	str_special[0b000000] = "sll";
	str_special[0b100101] = "or";
	str_special[0b101011] = "sltu";
	str_special[0b100001] = "addu";
	str_special[0b001000] = "jr";
	str_special[0b100100] = "and";
	str_special[0b100000] = "add";
	str_special[0b001001] = "jalr";
	str_special[0b100011] = "subu";
	str_special[0b000011] = "sra";
	str_special[0b011010] = "div";
	str_special[0b010010] = "mflo";
	str_special[0b000010] = "srl";
	str_special[0b011011] = "divu";
	str_special[0b010000] = "mfhi";
	str_special[0b101010] = "slt";
	str_special[0b001100] = "syscall";
	str_special[0b010011] = "mtlo";
	str_special[0b010001] = "mthi";
	str_special[0b000100] = "sllv";
	str_special[0b100111] = "nor";
	str_special[0b000111] = "srav";
	str_special[0b000110] = "srlv";
	str_special[0b011001] = "multu";
	str_special[0b100110] = "xor";
	str_special[0b001101] = "break";
	str_special[0b011000] = "mult";
	str_special[0b100010] = "sub";
}
