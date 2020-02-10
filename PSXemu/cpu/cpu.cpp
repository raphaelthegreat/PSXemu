#include "cpu.h"
#include <cpu/util.h>
#include <video/renderer.h>

CPU::CPU(Renderer* _renderer) : 
	load(std::make_pair(0, 0))
{
	renderer = _renderer;

	memset(regs, 0, 32);
	memset(out_regs, 0, 32);
	memset(cop0.regs, 0, 64 * 4);

	/* Create memory bus and load the bios. */
	bus = new Bus("SCPH1001.BIN", renderer);
	
	/* Register opcodes. */
	register_opcodes();

	/* Reset CPU. */
	reset();
}

CPU::~CPU()
{
	delete bus;
}

void CPU::reset()
{
	/* Reset CPU state. */
	pc = 0xbfc00000;
	next_pc = pc + 4;
	current_pc = 0;
	hi = 0; lo = 0;
	regs[0] = 0;

	load = std::make_pair(0, 0);
	should_branch = false; delay_slot = false;
}

void CPU::tick()
{
	current_pc = pc;
	
	/* Check for alignment errors. */
	if (current_pc % 4 != 0) {
		exception(ReadError);
		return;
	}
	
	/* Fetch instruction either from memory or from cahe. */
	fetch();

	pc = next_pc;
	next_pc = pc + 4;

	/* Apply any pending load delay slot. */
	auto [reg, val] = load;
	set_reg(reg, val);
	load = std::make_pair(0, 0);

	delay_slot = should_branch;
	should_branch = false;

	/* Execute fetched instruction. */
	auto& handler = lookup[instr.r_type.opcode];
	handler();
	
	/* Copy modified registers. */
	std::copy(out_regs, out_regs + 32, regs);
}

void CPU::fetch()
{
	/* Get cache control register. */
	CacheControl& cc = bus->cache_ctrl;

	/* Get PC address. */
	Address addr;
	addr.raw = pc;

	bool kseg1 = KSEG1.contains(pc).has_value();
	if (!kseg1 && cc.is1) {

		/* Fetch cache line and check it's validity. */
		CacheLine& line = instr_cache[addr.cache_line];		
		bool not_valid = line.tag.tag != addr.tag;
		not_valid |= line.tag.index > addr.index;

		/* Handle cache miss. */
		if (not_valid) {
			uint32_t cpc = pc;
			
			/* Move adjacent data to the cache. */
			for (int i = addr.index; i < 4; i++) {
				line.instrs[i].raw = read(cpc);
				cpc += 4;
			}
		}

		/* Validate the cache line. */
		line.tag.raw = pc & 0xfffff00c;

		/* Get instruction from cache. */
		instr = line.instrs[addr.index];
	}
	else {
		/* Fetch instruction from main RAM. */
		instr.raw = read(pc);
	}
}

uint32_t CPU::get_reg(uint32_t reg)
{
	return regs[reg];
}

void CPU::set_reg(uint32_t reg, uint32_t val)
{
	out_regs[reg] = val;
	out_regs[0] = 0;
}

void CPU::branch(uint32_t offset)
{
	offset = offset << 2;
	next_pc = pc + offset;
	should_branch = true;
}

void CPU::exception(ExceptionType type)
{
	uint32_t handler = 0;
	switch (cop0.sr.BEV) {
		case true: 
			handler = 0xbfc00180; 
			break;
		case false: 
			handler = 0x80000080; 
			break;
	}

	/* Get the Kernel-User mode bits. */
	uint32_t mode = cop0.sr.raw & 0x3f;
	/* Clear them. */
	cop0.sr.raw &= ~0x3f;
	/* Shift the to the left. */
	cop0.sr.raw |= (mode << 2) & 0x3f;
	/* Update the exec code. */
	cop0.cause.exc_code = type;

	/* Update the CAUSE register. */
	if (delay_slot) {
		cop0.epc = pc - 4;
		cop0.cause.BD = true;
	}
	else {
		cop0.epc = pc;
		cop0.cause.BD = false;
	}

	/* Update PC to exception handler. */
	pc = handler; 
	next_pc = handler + 4;
}

void CPU::op_lui()
{
	auto data = instr.i_type.data;
	auto rt = instr.r_type.rt;

	uint32_t val = data << 16;
	set_reg(rt, val);
}

void CPU::op_ori()
{
	auto data = instr.i_type.data;
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;

	uint32_t val = get_reg(rs) | data;
	set_reg(rt, val);
}

void CPU::op_sw()
{
	auto data = (int16_t)instr.i_type.data;
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;

	uint32_t addr = get_reg(rs) + data;
	
	if (addr % 4 == 0)
		write(addr, get_reg(rt));
	else
		exception(WriteError);
}

void CPU::op_lw()
{
	auto data = (int16_t)instr.i_type.data;
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;

	uint32_t s = get_reg(rs);
	uint32_t addr = s + data;
	
	if (addr % 4 == 0) {
		uint32_t val = read(addr);
		load = std::make_pair(rt, val);
	}
	else {
		exception(ReadError);
	}
}

void CPU::op_sll()
{
	auto data = instr.r_type.shamt;
	auto rt = instr.r_type.rt;
	auto rd = instr.r_type.rd;

	uint32_t val = get_reg(rt) << data;
	set_reg(rd, val);
}

void CPU::op_addiu()
{
	auto data = (int16_t)instr.i_type.data;
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;

	uint32_t val = get_reg(rs) + data;
	set_reg(rt, val);
}

void CPU::op_addi()
{
	int16_t data = instr.i_type.data;
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;
	
	uint32_t v = get_reg(rs);
	uint32_t result = v + data;
	bool overflow = ((~(v ^ data)) & (v ^ result)) & 0x80000000;

	if (overflow) {
		exception(Overflow);
		return;
	}

	set_reg(rt, result);
}

void CPU::op_j()
{
	uint32_t addr = instr.j_type.target;
	next_pc = (pc & 0xf0000000) | (addr << 2);
	
	should_branch = true;
}

void CPU::op_or()
{
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;
	auto rd = instr.r_type.rd;

	uint32_t val = get_reg(rs) | get_reg(rt);
	set_reg(rd, val);
}

void CPU::op_cop0()
{
	switch (instr.r_type.rs) {
		case 0b00000: 
			op_mfc0(); 
			break;
		case 0b00100: 
			op_mtc0(); 
			break;
		case 0b10000: 
			op_rfe(); 
			break;
		default: 
			panic("Cop0 unhanled opcode: 0x", instr.r_type.rs);
	}
}

void CPU::op_mfc0()
{
	auto cpu_r = instr.r_type.rt;
	auto cop_r = instr.r_type.rd;
	
	uint32_t val = cop0.regs[cop_r];
	load = std::make_pair(cpu_r, val);
}

void CPU::op_mtc0()
{
	auto cpu_r = instr.r_type.rt;
	auto cop_r = instr.r_type.rd;

	uint32_t val = get_reg(cpu_r);
	cop0.regs[cop_r] = val;
}

void CPU::op_cop1()
{
	exception(CopError);
}

void CPU::op_cop2()
{
	panic("Unhandled GTE instruction: ", instr.raw);
}

void CPU::op_cop3()
{
	exception(CopError);
}

void CPU::op_bne()
{
	auto offset = (int16_t)instr.i_type.data;
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;

	if (get_reg(rs) != get_reg(rt))
		branch(offset);
}

void CPU::op_break()
{
	exception(Break);
}

void CPU::op_syscall()
{
	exception(SysCall);
}

void CPU::op_rfe()
{
	auto mode = cop0.sr.raw & 0x3f;
	cop0.sr.raw &= ~0x3f;
	cop0.sr.raw |= (mode >> 2);
}

void CPU::op_lhu()
{
	auto data = (int16_t)instr.i_type.data;
	auto rt = instr.r_type.rt;
	auto base = instr.r_type.rs;

	uint32_t addr = get_reg(base) + data;
	
	if (addr % 2 == 0) {
		uint16_t val = read<uint16_t>(addr);
		load = std::make_pair(rt, (uint32_t)val);
	}
	else {
		exception(ReadError);
	}
}

void CPU::op_lh()
{
	auto offset = (int16_t)instr.i_type.data;
	auto base = instr.r_type.rs;
	auto rt = instr.r_type.rt;

	uint32_t addr = get_reg(base) + offset;

	if (addr % 2 == 0) {
		int16_t val = (int16_t)read<uint16_t>(addr);
		load = std::make_pair(rt, (uint32_t)val);
	}
	else {
		exception(ReadError);
	}
}

void CPU::op_lwl()
{
	auto data = (int16_t)instr.i_type.data;
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;

	uint32_t addr = get_reg(rs) + data;
	uint32_t cur_v = out_regs[rt];

	uint32_t aligned_addr = addr & ~3;
	uint32_t aligned_word = read(aligned_addr);

	uint32_t val;
	switch (addr & 0x3f) {
		case 0: 
			val = (cur_v & 0x00ffffff) | (aligned_word << 24); 
			break;
		case 1: 
			val = (cur_v & 0x0000ffff) | (aligned_word << 16); 
			break;
		case 2: 
			val = (cur_v & 0x000000ff) | (aligned_word << 8); 
			break;
		case 3: 
			val = (cur_v & 0x00000000) |  aligned_word; 
			break;
	};

	load = std::make_pair(rt, val);
}

void CPU::op_lwr()
{
	auto data = (int16_t)instr.i_type.data;
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;

	uint32_t addr = get_reg(rs) + data;
	uint32_t cur_v = out_regs[rt];

	uint32_t aligned_addr = addr & ~3;
	uint32_t aligned_word = read(aligned_addr);

	uint32_t val;
	switch (addr & 0x3f) {
		case 0: 
			val = (cur_v & 0x00000000) | aligned_word; 
			break;
		case 1: 
			val = (cur_v & 0xff000000) | (aligned_word >> 8);
			break;
		case 2: 
			val = (cur_v & 0xffff0000) | (aligned_word >> 16); 
			break;
		case 3: 
			val = (cur_v & 0xffffff00) | (aligned_word >> 24); 
			break;
	};

	load = std::make_pair(rt, val);
}

void CPU::op_swl()
{
	auto data = (int16_t)instr.i_type.data;
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;

	uint32_t addr = get_reg(rs) + data;
	uint32_t val = get_reg(rt);

	uint32_t aligned_addr = addr & ~3;
	uint32_t cur_mem = read(aligned_addr);

	uint32_t memory;
	switch (addr & 3) {
		case 0: 
			memory = (cur_mem & 0xffffff00) | (val >> 24); 
			break;
		case 1: 
			memory = (cur_mem & 0xffff0000) | (val >> 16); 
			break;
		case 2: 
			memory = (cur_mem & 0xff000000) | (val >> 8); 
			break;
		case 3: 
			memory = (cur_mem & 0x00000000) | val; 
			break;
	};

	write<uint32_t>(aligned_addr, memory);
}

void CPU::op_swr()
{
	auto data = (int16_t)instr.i_type.data;
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;

	uint32_t addr = get_reg(rs) + data;
	uint32_t val = get_reg(rt);

	uint32_t aligned_addr = addr & ~3;
	uint32_t cur_mem = read(aligned_addr);

	uint32_t memory;
	switch (addr & 3) {
		case 0: 
			memory = (cur_mem & 0x00000000) | val; 
			break;
		case 1: 
			memory = (cur_mem & 0x000000ff) | (val << 8); 
			break;
		case 2: 
			memory = (cur_mem & 0x0000ffff) | (val << 16); 
			break;
		case 3: 
			memory = (cur_mem & 0x00ffffff) | (val << 24); 
			break;
	};

	write<uint32_t>(aligned_addr, memory);
}

void CPU::op_lwc0()
{
	exception(CopError);
}

void CPU::op_lwc1()
{
	exception(CopError);
}

void CPU::op_lwc2()
{
	panic("Unhandled read from Cop2!", "");
}

void CPU::op_lwc3()
{
	exception(CopError);
}

void CPU::op_swc0()
{
	exception(CopError);
}

void CPU::op_swc1()
{
	exception(CopError);
}

void CPU::op_swc2()
{
	panic("Unhandled write to Cop2!", "");
}

void CPU::op_swc3()
{
	exception(CopError);
}

void CPU::op_illegal()
{
	exception(IllegalInstr);
}

void CPU::op_sltu()
{
	auto rd = instr.r_type.rd;
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;

	uint32_t val = get_reg(rs) < get_reg(rt);
	set_reg(rd, val);
}

void CPU::op_addu()
{
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;
	auto rd = instr.r_type.rd;

	uint32_t v = get_reg(rs) + get_reg(rt);
	set_reg(rd, v);
}

void CPU::op_sh()
{
	auto offset = (int16_t)instr.i_type.data;
	auto base = instr.r_type.rs;
	auto rt = instr.r_type.rt;

	uint32_t addr = get_reg(base) + offset;
	uint16_t data = (uint16_t)get_reg(rt);
	
	if (addr % 2 == 0)
		write<uint16_t>(addr, data);
	else
		exception(WriteError);
}

void CPU::op_jal()
{
	set_reg(31, next_pc);
	op_j();
}

void CPU::op_andi()
{
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;
	auto imm = instr.i_type.data;

	uint32_t val = get_reg(rs) & imm;
	set_reg(rt, val);
}

void CPU::op_sb()
{
	auto offset = (int16_t)instr.i_type.data;
	auto rt = instr.r_type.rt;
	auto base = instr.r_type.rs;

	uint32_t addr = get_reg(base) + offset;
	uint8_t data = (uint8_t)get_reg(rt);

	write<uint8_t>(addr, data);
}

void CPU::op_jr()
{
	auto rs = instr.r_type.rs;
	next_pc = get_reg(rs);

	should_branch = true;
}

void CPU::op_lb()
{
	auto offset = (int16_t)instr.i_type.data;
	auto base = instr.r_type.rs;
	auto rt = instr.r_type.rt;

	uint32_t addr = get_reg(base) + offset;
	int8_t data = (int8_t)read<uint8_t>(addr);
	load = std::make_pair(rt, data);
}

void CPU::op_beq()
{
	auto offset = (int16_t)instr.i_type.data;
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;

	if (get_reg(rs) == get_reg(rt))
		branch(offset);
}

void CPU::op_and()
{
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;
	auto rd = instr.r_type.rd;

	uint32_t val = get_reg(rs) & get_reg(rt);
	set_reg(rd, val);
}

void CPU::op_add()
{
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;
	auto rd = instr.r_type.rd;

	uint32_t v1 = get_reg(rs);
	uint32_t v2 = get_reg(rt);
	
	uint32_t result = v1 + v2;
	bool overflow = ((~(v1 ^ v2)) & (v1 ^ result)) & 0x80000000;

	if (overflow) {
		exception(Overflow);
		return;
	}

	set_reg(rd, result);
}

void CPU::op_bgtz()
{
	auto offset = (int16_t)instr.i_type.data;
	auto rs = instr.r_type.rs;

	int32_t val = (int32_t)get_reg(rs);
	if (val > 0)
		branch(offset);
}

void CPU::op_blez()
{
	auto offset = (int16_t)instr.i_type.data;
	auto rs = instr.r_type.rs;
	
	int32_t val = (int32_t)get_reg(rs);
	if (val <= 0)
		branch(offset);
}

void CPU::op_lbu()
{
	auto data = (int16_t)instr.i_type.data;
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;

	uint32_t addr = get_reg(rs) + data;
	uint8_t val = read<uint8_t>(addr);

	load = std::make_pair(rt, (uint32_t)val);
}

void CPU::op_jalr()
{
	auto rd = instr.r_type.rd;
	auto rs = instr.r_type.rs;

	set_reg(rd, next_pc);
	next_pc = get_reg(rs);

	should_branch = true;
}

void CPU::op_bxx()
{
	auto i = (int16_t)instr.i_type.data;
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;

	bool isbgez = instr.r_type.rt & 1;
	bool islink = ((instr.raw >> 17) & 0xf) == 8;
	int32_t v = (int32_t)get_reg(rs);

	uint32_t test = v < 0;
	test ^= isbgez;

	if (islink)
		set_reg(31, next_pc);

	if (test != 0) {
		branch(i);
	}
}

void CPU::op_slti()
{
	int32_t i = (int16_t)instr.i_type.data;
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;

	bool v = (int32_t)get_reg(rs) < i;
	set_reg(rt, (uint32_t)v);
}

void CPU::op_subu()
{
	auto rd = instr.r_type.rd;
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;

	uint32_t val = get_reg(rs) - get_reg(rt);
	set_reg(rd, val);
}

void CPU::op_sra()
{
	auto i = instr.r_type.shamt;
	auto rd = instr.r_type.rd;
	auto rt = instr.r_type.rt;

	auto  val = (int32_t)get_reg(rt) >> i;
	set_reg(rd, (uint32_t)val);
}

void CPU::op_div()
{
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;

	auto n = (int32_t)get_reg(rs);
	auto d = (int32_t)get_reg(rt);

	if (d == 0) {
		// Division by zero, results are bogus
		hi = (uint32_t)n;

		if (n >= 0) {
			lo = 0xffffffff;
		}
		else {
			lo = 1;
		}
	}
	else if ((uint32_t)n == 0x80000000 && d == -1) {
		// Result is not representable in a 32bit signed integer
		hi = 0;
		lo = 0x80000000;
	}
	else {
		hi = (uint32_t)(n % d);
		lo = (uint32_t)(n / d);
	}
}

void CPU::op_mlfo()
{
	auto rd = instr.r_type.rd;
	set_reg(rd, lo);
}

void CPU::op_nor()
{
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;
	auto rd = instr.r_type.rd;

	uint32_t val = !(get_reg(rs) | get_reg(rt));
	set_reg(rd, val);
}

void CPU::op_slt()
{
	auto rd = instr.r_type.rd;
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;

	int32_t t = static_cast<int32_t>(get_reg(rt));
	int32_t s = static_cast<int32_t>(get_reg(rs));

	uint32_t val = s < t;
	set_reg(rd, val);
}

void CPU::op_sltiu()
{
	auto data = (int16_t)instr.i_type.data;
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;

	uint32_t val = get_reg(rs) < data;
	set_reg(rt, val);
}

void CPU::op_sub()
{
	auto rd = instr.r_type.rd;
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;

	int64_t val = (int64_t)get_reg(rs) - (int64_t)get_reg(rt);
	if (val < INT_MIN || val > INT_MAX) exception(Overflow);
	set_reg(rd, (uint32_t)val);
}

void CPU::op_xor()
{
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;
	auto rd = instr.r_type.rd;

	uint32_t val = get_reg(rs) ^ get_reg(rt);
	set_reg(rd, val);
}

void CPU::op_xori()
{
	auto rs = instr.r_type.rs;
	auto rt = instr.r_type.rt;
	auto data = instr.i_type.data;

	uint32_t val = get_reg(rs) ^ data;
	set_reg(rt, val);
}

void CPU::op_sllv()
{
	auto rd = instr.r_type.rd;
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;

	uint32_t val = get_reg(rt) << (get_reg(rs) & 0x1f);
	set_reg(rd, val);
}

void CPU::op_srav()
{
	auto rd = instr.r_type.rd;
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;
	
	int32_t val = (int32_t)get_reg(rt) >> (get_reg(rs) & 0x1f);
	set_reg(rd, (uint32_t)val);
}

void CPU::op_srl()
{
	auto i = instr.r_type.shamt;
	auto rt = instr.r_type.rt;
	auto rd = instr.r_type.rd;

	uint32_t val = get_reg(rt) >> i;
	set_reg(rd, val);
}

void CPU::op_srlv()
{
	auto rd = instr.r_type.rd;
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;

	uint32_t val = get_reg(rt) >> (get_reg(rs) & 0x1f);
	set_reg(rd, val);
}

void CPU::op_divu()
{
	auto s = instr.r_type.rs;
	auto t = instr.r_type.rt;
	
	auto n = get_reg(s);
	auto d = get_reg(t);
	
	if (d == 0) {
		hi = n;
		lo = 0xffffffff;
	}
	else {
		hi = n % d;
		lo = n / d;
	}
}

void CPU::op_mfhi()
{
	auto rd = instr.r_type.rd;
	set_reg(rd, hi);
}

void CPU::op_mflo()
{
	auto rd = instr.r_type.rd;
	set_reg(rd, lo);
}

void CPU::op_mthi()
{
	auto rs = instr.r_type.rs;
	hi = get_reg(rs);
}

void CPU::op_mtlo()
{
	auto rs = instr.r_type.rs;
	lo = get_reg(rs);
}

void CPU::op_mult()
{
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;

	int64_t v1 = (int32_t)get_reg(rs);
	int64_t v2 = (int32_t)get_reg(rt);
	uint64_t result = v1 * v2;
	
	hi = result >> 32;
	lo = result;
}

void CPU::op_multu()
{
	auto rt = instr.r_type.rt;
	auto rs = instr.r_type.rs;

	uint64_t v1 = get_reg(rs);
	uint64_t v2 = get_reg(rt);
	uint64_t result = v1 * v2;
	
	hi = result >> 32;
	lo = result;
}