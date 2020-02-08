#include "cpu.h"
#include <cpu/util.h>
#include <video/renderer.h>
#include <memory/interconnect.h>

using std::begin;
using std::end;

CPU::CPU(Renderer* _renderer) : load(std::make_pair(0, 0))
{
	renderer = _renderer;

	memset(regs, 0xdeadbeef, 32);
	memset(out_regs, 0xdeadbeef, 32);

	inter = std::make_unique<Interconnect>("SCPH1001.BIN", renderer);
	this->reset();
}

void CPU::reset()
{
	pc = 0xbfc00000;
	next_pc = pc + 4;
	current_pc = 0;
	sr = 0; cause = 0; epc = 0;
	hi = 0; low = 0;
	regs[0] = 0;

	load = std::make_pair(0, 0);
	isbranch = false; isdelay_slot = false;
}

void CPU::tick()
{
	auto instr = Instruction(read(pc));
	
 	current_pc = pc;
	pc = next_pc; 
	next_pc = pc + 4;

	//printf("0x%x\n", pc);

	if (current_pc % 4 != 0) {
		exception(ExceptionType::ReadAddressError);
		return;
	}

	auto [reg, val] = load;
	set_reg(reg, val);

	load = std::make_pair(0, 0);

	isdelay_slot = isbranch;
	isbranch = false;

	execute(instr);
	for (int i = 0; i < 32; i++) regs[i] = out_regs[i];
}

void CPU::execute(Instruction instr)
{
	switch (instr.type()) {
		case 0b000000:
			switch (instr.subtype()) {
				case 0b000000: op_sll(instr); break;
				case 0b100101: op_or(instr); break;
				case 0b101011: op_sltu(instr); break;
				case 0b100001: op_addu(instr); break;
				case 0b001000: op_jr(instr); break;
				case 0b100100: op_and(instr); break;
				case 0b100000: op_add(instr); break;
				case 0b001001: op_jalr(instr); break;
				case 0b100011: op_subu(instr); break;
				case 0b000011: op_sra(instr); break;
				case 0b011010: op_div(instr); break;
				case 0b010010: op_mlfo(instr); break;
				case 0b000010: op_srl(instr); break;
				case 0b011011: op_divu(instr); break;
				case 0b010000: op_mfhi(instr); break;
				case 0b101010: op_slt(instr); break;
				case 0b001100: op_syscall(instr); break;
				case 0b010011: op_mtlo(instr); break;
				case 0b010001: op_mthi(instr); break;
				case 0b000100: op_sllv(instr); break;
				case 0b100111: op_nor(instr); break;
				case 0b000111: op_srav(instr); break;
				case 0b000110: op_srlv(instr); break;
				case 0b011001: op_multu(instr); break;
				case 0b100110: op_xor(instr); break;
				case 0b001101: op_break(instr); break;
				case 0b011000: op_multu(instr); break;
				case 0b100010: op_sub(instr); break;
				default: op_illegal(instr); break;
			}
			break;
		case 0b001111: op_lui(instr); break;
		case 0b001101: op_ori(instr); break;
		case 0b101011: op_sw(instr); break;
		case 0b001001: op_addiu(instr); break;
		case 0b001000: op_addi(instr); break;
		case 0b000010: op_j(instr); break;
		case 0b010000: op_cop0(instr); break;
		case 0b010001: op_cop1(instr); break;
		case 0b010010: op_cop2(instr); break;
		case 0b010011: op_cop3(instr); break;
		case 0b100011: op_lw(instr); break;
		case 0b000101: op_bne(instr); break;
		case 0b101001: op_sh(instr); break;
		case 0b000011: op_jal(instr); break;
		case 0b001100: op_andi(instr); break;
		case 0b101000: op_sb(instr); break;
		case 0b100000: op_lb(instr); break;
		case 0b000100: op_beq(instr); break;
		case 0b000111: op_bgtz(instr); break;
		case 0b000110: op_blez(instr); break;
		case 0b100100: op_lbu(instr); break;
		case 0b000001: op_bxx(instr); break;
		case 0b001010: op_slti(instr); break;
		case 0b001011: op_sltiu(instr); break;
		case 0b100101: op_lhu(instr); break;
		case 0b100001: op_lh(instr); break;
		case 0b001110: op_xori(instr); break;
		case 0b110000: op_lwc0(instr); break;
		case 0b110001: op_lwc1(instr); break;
		case 0b110010: op_lwc2(instr); break;
		case 0b110011: op_lwc3(instr); break;
		case 0b111000: op_swc0(instr); break;
		case 0b111001: op_swc1(instr); break;
		case 0b111010: op_swc2(instr); break;
		case 0b101110: op_swr(instr); break;
		case 0b111011: op_swc3(instr); break;
		case 0b101010: op_swl(instr); break;
		case 0b100010: op_lwl(instr); break;
		case 0b100110: op_lwr(instr); break;
		default: op_illegal(instr); break;
	}
}

uint32_t CPU::get_reg(RegIndex reg)
{
	return regs[reg.index];
}

void CPU::set_reg(RegIndex reg, uint32_t val)
{
	out_regs[reg.index] = val;
	out_regs[0] = 0;
}

void CPU::branch(uint32_t offset)
{
	offset = offset << 2;
	next_pc = pc + offset;
	isbranch = true;
}

void CPU::exception(ExceptionType type)
{
	uint32_t handler;
	switch ((sr & (1 << 22)) != 0) {
	case true: handler = 0xbfc00180; break;
	case false: handler = 0x80000080; break;
	}

	uint32_t mode = sr & 0x3f; // Get mode
	sr &= ~0x3f; // Clear mode bits
	sr |= (mode << 2) & 0x3f;

	cause = (uint32_t)type << 2;
	epc = current_pc;

	if (isdelay_slot) {
		epc -= 4;
		set_bit(cause, 31, true);
	}

	pc = handler; next_pc = handler + 4;
}

bool CPU::cache_write()
{
	return get_bit(sr, 16);
}

void CPU::op_lui(Instruction instr)
{
	auto data = instr.imm();
	auto rt = instr.rt();

	uint32_t val = data << 16;
	set_reg(rt, val);
}

void CPU::op_ori(Instruction instr)
{
	auto data = instr.imm();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t val = get_reg(rs) | data;
	set_reg(rt, val);
}

void CPU::op_sw(Instruction instr)
{
	auto data = instr.simm();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t addr = get_reg(rs) + data;
	
	if (addr % 4 == 0)
		write<uint32_t>(addr, get_reg(rt));
	else
		exception(ExceptionType::WriteAddressError);
}

void CPU::op_lw(Instruction instr)
{
	auto data = instr.simm();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t s = get_reg(rs);
	uint32_t addr = s + data;
	
	if (addr % 4 == 0)
		load = std::make_pair(rt, read<uint32_t>(addr));
	else
		exception(ExceptionType::ReadAddressError);
}

void CPU::op_sll(Instruction instr)
{
	auto data = instr.shift();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t val = get_reg(rt) << data;
	set_reg(rd, val);
}

void CPU::op_addiu(Instruction instr)
{
	auto data = instr.simm();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t val = get_reg(rs) + data;
	set_reg(rt, val);
}

void CPU::op_addi(Instruction instr)
{
	int32_t data = instr.simm();
	auto rt = instr.rt();
	auto rs = instr.rs();

	int32_t s = get_reg(rs);
	int64_t sum = (int64_t)s + data;
	if (sum < INT_MIN || sum > INT_MAX) exception(ExceptionType::Overflow);

	set_reg(rt, (uint32_t)sum);
}

void CPU::op_j(Instruction instr)
{
	uint32_t addr = instr.imm_jump();
	next_pc = (pc & 0xf0000000) | (addr << 2);
	
	isbranch = true;
}

void CPU::op_or(Instruction instr)
{
	auto rs = instr.rs();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t val = get_reg(rs) | get_reg(rt);
	set_reg(rd, val);
}

void CPU::op_cop0(Instruction instr)
{
	switch (instr.cop_op()) {
	case 0b00000: op_mfc0(instr); break;
	case 0b00100: op_mtc0(instr); break;
	case 0b10000: op_rfe(instr); break;
	default: panic("Cop0 unhanled instruction: 0x", instr.cop_op());
	}
}

void CPU::op_mfc0(Instruction instr)
{
	auto cpu_r = instr.rt();
	auto cop_r = instr.rd().index;
	
	uint32_t val;
	switch (cop_r) {
	case 12: val = sr; break;
	case 13: val = cause; break;
	case 14: val = epc; break;
	default: panic("Unheandled read from Cop0 register: 0x", cop_r);
	}

	load = std::make_pair(cpu_r, val);
}

void CPU::op_mtc0(Instruction instr)
{
	auto cpu_r = instr.rt();
	auto cop_r = instr.rd().index;

	uint32_t val = get_reg(cpu_r);
	
	switch (cop_r) {
	case 3:
	case 5:
	case 6:
	case 7:
	case 9:
	case 11:
		if (val != 0)
			panic("Unhandled write to cop0 register: ", cop_r);
		break;
	case 12: sr = val; break;
	case 13:
		if (val != 0)
			panic("Unhandled write to cop0 CAUSE register: ", cop_r);
		break;
	default: panic("Unhandled cop0 register: ", cop_r);
	}
}

void CPU::op_cop1(Instruction instr)
{
	exception(ExceptionType::CopError);
}

void CPU::op_cop2(Instruction instr)
{
	panic("Unhandled GTE instruction: ", instr.opcode);
}

void CPU::op_cop3(Instruction instr)
{
	exception(ExceptionType::CopError);
}

void CPU::op_bne(Instruction instr)
{
	auto offset = instr.simm();
	auto rs = instr.rs();
	auto rt = instr.rt();

	if (get_reg(rs) != get_reg(rt))
		branch(offset);
}

void CPU::op_break(Instruction instr)
{
	exception(ExceptionType::Break);
}

void CPU::op_syscall(Instruction instr)
{
	exception(ExceptionType::SysCall);
}

void CPU::op_rfe(Instruction instr)
{
	uint8_t bits = instr.opcode & 0x3f;
	if (bits != 0b010000)
		panic("Virtual memory call on RFE instruction!", "");

	auto mode = sr & 0x3f;
	sr &= !0x3f;
	sr |= (mode >> 2);
}

void CPU::op_lhu(Instruction instr)
{
	auto data = instr.simm();
	auto rt = instr.rt();
	auto base = instr.rs();

	uint32_t addr = get_reg(base) + data;
	
	if (addr % 2 == 0)
		load = std::make_pair(rt, (uint32_t)read<uint16_t>(addr));
	else
		exception(ExceptionType::ReadAddressError);
}

void CPU::op_lh(Instruction instr)
{
	auto offset = instr.simm();
	auto base = instr.rs();
	auto rt = instr.rt();

	uint32_t addr = get_reg(base) + offset;
	int16_t val = static_cast<int16_t>(read<uint16_t>(addr));
	if (addr % 2 == 0)
		load = std::make_pair(rt, (uint32_t)val);
	else
		exception(ExceptionType::ReadAddressError);
}

void CPU::op_lwl(Instruction instr)
{
	auto data = instr.simm();
	auto rs = instr.rs();
	auto rt = instr.rt();

	uint32_t addr = get_reg(rs) + data;
	uint32_t cur_v = out_regs[rt.index];

	uint32_t aligned_addr = addr & ~3;
	uint32_t aligned_word = read<uint32_t>(aligned_addr);

	uint32_t val;
	switch (addr & 0x3f) {
	case 0: val = (cur_v & 0x00ffffff) | (aligned_word << 24); break;
	case 1: val = (cur_v & 0x0000ffff) | (aligned_word << 16); break;
	case 2: val = (cur_v & 0x000000ff) | (aligned_word << 8); break;
	case 3: val = (cur_v & 0x00000000) |  aligned_word; break;
	};

	load = std::make_pair(rt, val);
}

void CPU::op_lwr(Instruction instr)
{
	auto data = instr.simm();
	auto rs = instr.rs();
	auto rt = instr.rt();

	uint32_t addr = get_reg(rs) + data;
	uint32_t cur_v = out_regs[rt.index];

	uint32_t aligned_addr = addr & ~3;
	uint32_t aligned_word = read<uint32_t>(aligned_addr);

	uint32_t val;
	switch (addr & 0x3f) {
	case 0: val = (cur_v & 0x00000000) | aligned_word; break;
	case 1: val = (cur_v & 0xff000000) | (aligned_word >> 8); break;
	case 2: val = (cur_v & 0xffff0000) | (aligned_word >> 16); break;
	case 3: val = (cur_v & 0xffffff00) | (aligned_word >> 24); break;
	};

	load = std::make_pair(rt, val);
}

void CPU::op_swl(Instruction instr)
{
	auto data = instr.simm();
	auto rs = instr.rs();
	auto rt = instr.rt();

	uint32_t addr = get_reg(rs) + data;
	uint32_t val = get_reg(rt);

	uint32_t aligned_addr = addr & ~3;
	uint32_t cur_mem = read<uint32_t>(aligned_addr);

	uint32_t memory;
	switch (addr & 3) {
	case 0: memory = (cur_mem & 0xffffff00) | (val >> 24); break;
	case 1: memory = (cur_mem & 0xffff0000) | (val >> 16); break;
	case 2: memory = (cur_mem & 0xff000000) | (val >> 8); break;
	case 3: memory = (cur_mem & 0x00000000) | val; break;
	};

	write<uint32_t>(aligned_addr, memory);
}

void CPU::op_swr(Instruction instr)
{
	auto data = instr.simm();
	auto rs = instr.rs();
	auto rt = instr.rt();

	uint32_t addr = get_reg(rs) + data;
	uint32_t val = get_reg(rt);

	uint32_t aligned_addr = addr & ~3;
	uint32_t cur_mem = read<uint32_t>(aligned_addr);

	uint32_t memory;
	switch (addr & 3) {
	case 0: memory = (cur_mem & 0x00000000) | val; break;
	case 1: memory = (cur_mem & 0x000000ff) | (val << 8); break;
	case 2: memory = (cur_mem & 0x0000ffff) | (val << 16); break;
	case 3: memory = (cur_mem & 0x00ffffff) | (val << 24); break;
	};

	write<uint32_t>(aligned_addr, memory);
}

void CPU::op_lwc0(Instruction instr)
{
	exception(ExceptionType::CopError);
}

void CPU::op_lwc1(Instruction instr)
{
	exception(ExceptionType::CopError);
}

void CPU::op_lwc2(Instruction instr)
{
	panic("Unhandled read from Cop2!", "");
}

void CPU::op_lwc3(Instruction instr)
{
	exception(ExceptionType::CopError);
}

void CPU::op_swc0(Instruction instr)
{
	exception(ExceptionType::CopError);
}

void CPU::op_swc1(Instruction instr)
{
	exception(ExceptionType::CopError);
}

void CPU::op_swc2(Instruction instr)
{
	panic("Unhandled write to Cop2!", "");
}

void CPU::op_swc3(Instruction instr)
{
	exception(ExceptionType::CopError);
}

void CPU::op_illegal(Instruction instr)
{
	exception(ExceptionType::IllegalInstr);
}

void CPU::op_sltu(Instruction instr)
{
	auto rd = instr.rd();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t val = get_reg(rs) < get_reg(rt);
	set_reg(rd, val);
}

void CPU::op_addu(Instruction instr)
{
	auto rs = instr.rs();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t v = get_reg(rs) + get_reg(rt);
	set_reg(rd, v);
}

void CPU::op_sh(Instruction instr)
{
	auto offset = instr.simm();
	auto base = instr.rs();
	auto rt = instr.rt();

	uint32_t addr = get_reg(base) + offset;
	uint16_t data = static_cast<uint16_t>(get_reg(rt));
	
	if (addr % 2 == 0)
		write<uint16_t>(addr, data);
	else
		exception(ExceptionType::WriteAddressError);
}

void CPU::op_jal(Instruction instr)
{
	set_reg(31, next_pc);
	op_j(instr);
}

void CPU::op_andi(Instruction instr)
{
	auto rt = instr.rt();
	auto rs = instr.rs();
	auto imm = instr.imm();

	uint32_t val = get_reg(rs) & imm;
	set_reg(rt, val);
}

void CPU::op_sb(Instruction instr)
{
	auto offset = instr.simm();
	auto rt = instr.rt();
	auto base = instr.rs();

	uint32_t addr = get_reg(base) + offset;
	uint8_t data = static_cast<uint8_t>(get_reg(rt));
	write<uint8_t>(addr, data);
}

void CPU::op_jr(Instruction instr)
{
	auto rs = instr.rs();
	next_pc = get_reg(rs);

	isbranch = true;
}

void CPU::op_lb(Instruction instr)
{
	auto offset = instr.simm();
	auto base = instr.rs();
	auto rt = instr.rt();

	uint32_t addr = get_reg(base) + offset;
	int8_t data = (int8_t)read<uint8_t>(addr);
	load = std::make_pair(rt, data);
}

void CPU::op_beq(Instruction instr)
{
	auto offset = instr.simm();
	auto rs = instr.rs();
	auto rt = instr.rt();

	if (get_reg(rs) == get_reg(rt))
		branch(offset);
}

void CPU::op_and(Instruction instr)
{
	auto rs = instr.rs();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t val = get_reg(rs) & get_reg(rt);
	set_reg(rd, val);
}

void CPU::op_add(Instruction instr)
{
	auto rs = instr.rs();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t v = get_reg(rs) + get_reg(rt);
	set_reg(rd, v);

	int32_t s = get_reg(rs);
	int32_t t = get_reg(rt);

	int64_t sum = s + t;
	if (sum < INT_MIN || sum > INT_MAX) exception(ExceptionType::Overflow);

	set_reg(rd, (uint32_t)sum);
}

void CPU::op_bgtz(Instruction instr)
{
	auto offset = instr.simm();
	auto rs = instr.rs();

	int32_t val = static_cast<int32_t>(get_reg(rs));
	if (val > 0)
		branch(offset);
}

void CPU::op_blez(Instruction instr)
{
	auto offset = instr.simm();
	auto rs = instr.rs();
	
	int32_t val = static_cast<int32_t>(get_reg(rs));
	if (val <= 0)
		branch(offset);
}

void CPU::op_lbu(Instruction instr)
{
	auto data = instr.simm();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t addr = get_reg(rs) + data;
	uint8_t val = read<uint8_t>(addr);

	load = std::make_pair(rt, (uint32_t)val);
}

void CPU::op_jalr(Instruction instr)
{
	auto rd = instr.rd();
	auto rs = instr.rs();

	set_reg(rd, next_pc);
	next_pc = get_reg(rs);

	isbranch = true;
}

void CPU::op_bxx(Instruction instr)
{
	auto i = instr.simm();
	auto rt = instr.rt();
	auto rs = instr.rs();

	bool isbgez = (instr.opcode >> 16) & 1;
	bool islink = (instr.opcode >> 17) & 0xf == 8;
	int32_t v = static_cast<int32_t>(get_reg(rs));

	uint32_t test = v < 0;
	test ^= isbgez;

	if (islink)
		set_reg(31, next_pc);

	if (test != 0) {
		branch(i);
	}
}

void CPU::op_slti(Instruction instr)
{
	int32_t i = static_cast<int32_t>(instr.simm());
	auto rs = instr.rs();
	auto rt = instr.rt();

	auto v = static_cast<int32_t>(get_reg(rs)) < i;
	set_reg(rt, (uint32_t)v);
}

void CPU::op_subu(Instruction instr)
{
	auto rd = instr.rd();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t val = get_reg(rs) - get_reg(rt);
	set_reg(rd, val);
}

void CPU::op_sra(Instruction instr)
{
	auto i = instr.shift();
	auto rd = instr.rd();
	auto rt = instr.rt();

	auto  val = (int32_t)get_reg(rt) >> i;
	set_reg(rd, (uint32_t)val);
}

void CPU::op_div(Instruction instr)
{
	auto rs = instr.rs();
	auto rt = instr.rt();

	auto n = (int32_t)get_reg(rs);
	auto d = (int32_t)get_reg(rt);

	if (d == 0) {
		// Division by zero, results are bogus
		hi = (uint32_t)n;

		if (n >= 0) {
			low = 0xffffffff;
		}
		else {
			low = 1;
		}
	}
	else if ((uint32_t)n == 0x80000000 && d == -1) {
		// Result is not representable in a 32bit signed integer
		hi = 0;
		low = 0x80000000;
	}
	else {
		hi = (uint32_t)(n % d);
		low = (uint32_t)(n / d);
	}
}

void CPU::op_mlfo(Instruction instr)
{
	auto rd = instr.rd();
	set_reg(rd, low);
}

void CPU::op_nor(Instruction instr)
{
	auto rs = instr.rs();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t val = !(get_reg(rs) | get_reg(rt));
	set_reg(rd, val);
}

void CPU::op_slt(Instruction instr)
{
	auto rd = instr.rd();
	auto rs = instr.rs();
	auto rt = instr.rt();

	int32_t t = static_cast<int32_t>(get_reg(rt));
	int32_t s = static_cast<int32_t>(get_reg(rs));

	uint32_t val = s < t;
	set_reg(rd, val);
}

void CPU::op_sltiu(Instruction instr)
{
	auto data = instr.simm();
	auto rs = instr.rs();
	auto rt = instr.rt();

	uint32_t val = get_reg(rs) < data;
	set_reg(rt, val);
}

void CPU::op_sub(Instruction instr)
{
	auto rd = instr.rd();
	auto rt = instr.rt();
	auto rs = instr.rs();

	int64_t val = (int64_t)get_reg(rs) - (int64_t)get_reg(rt);
	if (val < INT_MIN || val > INT_MAX) exception(ExceptionType::Overflow);
	set_reg(rd, (uint32_t)val);
}

void CPU::op_xor(Instruction instr)
{
	auto rs = instr.rs();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t val = get_reg(rs) ^ get_reg(rt);
	set_reg(rd, val);
}

void CPU::op_xori(Instruction instr)
{
	auto rs = instr.rs();
	auto rt = instr.rt();
	auto data = instr.imm();

	uint32_t val = get_reg(rs) ^ data;
	set_reg(rt, val);
}

void CPU::op_sllv(Instruction instr)
{
	auto rd = instr.rd();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t val = get_reg(rt) << (get_reg(rs) & 0x1f);
	set_reg(rd, val);
}

void CPU::op_srav(Instruction instr)
{
	auto rd = instr.rd();
	auto rt = instr.rt();
	auto rs = instr.rs();
	
	int32_t val = (int32_t)get_reg(rt) >> (get_reg(rs) & 0x1f);
	set_reg(rd, (uint32_t)val);
}

void CPU::op_srl(Instruction instr)
{
	auto i = instr.shift();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t val = get_reg(rt) >> i;
	set_reg(rd, val);
}

void CPU::op_srlv(Instruction instr)
{
	auto rd = instr.rd();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t val = get_reg(rt) >> (get_reg(rs) & 0x1f);
	set_reg(rd, val);
}

void CPU::op_divu(Instruction instr)
{
	auto s = instr.rs();
	auto t = instr.rt();
	auto n = get_reg(s);
	auto d = get_reg(t);
	
	if (d == 0) {
		hi = n;
		low = 0xffffffff;
	}
	else {
		hi = n % d;
		low = n / d;
	}
}

void CPU::op_mfhi(Instruction instr)
{
	auto rd = instr.rd();
	set_reg(rd, hi);
}

void CPU::op_mflo(Instruction instr)
{
}

void CPU::op_mthi(Instruction instr)
{
	auto rs = instr.rs();
	hi = get_reg(rs);
}

void CPU::op_mtlo(Instruction instr)
{
	auto rs = instr.rs();
	low = get_reg(rs);
}

void CPU::op_mult(Instruction instr)
{
	auto rt = instr.rt();
	auto rs = instr.rs();

	int64_t t = static_cast<int64_t>((int32_t)get_reg(rt));
	int64_t s = static_cast<int64_t>((int32_t)get_reg(rs));

	uint64_t product = static_cast<int64_t>(t * s);
	low = static_cast<uint32_t>(product);
	hi = static_cast<uint32_t>(product >> 32);
}

void CPU::op_multu(Instruction instr)
{
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint64_t t = static_cast<uint64_t>(get_reg(rt));
	uint64_t s = static_cast<uint64_t>(get_reg(rs));

	uint64_t product = t * s;
	low = static_cast<uint32_t>(product);
	hi = static_cast<uint32_t>(product >> 32);
}