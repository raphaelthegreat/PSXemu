#include "cpu.h"
#include <cpu/util.h>
#include <video/renderer.h>

CPU::CPU(Renderer* _renderer) : 
	load(std::make_pair(0, 0)), instr(0)
{
	renderer = _renderer;

	memset(regs, 0, 32);
	memset(out_regs, 0, 32);
	memset(cop0.regs, 0, 64 * 4);

	/* Create memory bus and load the bios. */
	bus = new Bus("SCPH1001.BIN", renderer);
	this->reset();
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
	execute();
	
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
				line.instrs[i] = Instruction(read(cpc));
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
		instr.opcode = read(pc);
	}
}

void CPU::execute()
{
	/* Execute instruction. */
	/* TODO: change this to std::unordred_map. */
	switch (instr.type()) {
		case 0b000000:
			switch (instr.subtype()) {
				case 0b000000: op_sll(); break;
				case 0b100101: op_or(); break;
				case 0b101011: op_sltu(); break;
				case 0b100001: op_addu(); break;
				case 0b001000: op_jr(); break;
				case 0b100100: op_and(); break;
				case 0b100000: op_add(); break;
				case 0b001001: op_jalr(); break;
				case 0b100011: op_subu(); break;
				case 0b000011: op_sra(); break;
				case 0b011010: op_div(); break;
				case 0b010010: op_mlfo(); break;
				case 0b000010: op_srl(); break;
				case 0b011011: op_divu(); break;
				case 0b010000: op_mfhi(); break;
				case 0b101010: op_slt(); break;
				case 0b001100: op_syscall(); break;
				case 0b010011: op_mtlo(); break;
				case 0b010001: op_mthi(); break;
				case 0b000100: op_sllv(); break;
				case 0b100111: op_nor(); break;
				case 0b000111: op_srav(); break;
				case 0b000110: op_srlv(); break;
				case 0b011001: op_multu(); break;
				case 0b100110: op_xor(); break;
				case 0b001101: op_break(); break;
				case 0b011000: op_multu(); break;
				case 0b100010: op_sub(); break;
				default: op_illegal(); break;
			}
			break;
		case 0b001111: op_lui(); break;
		case 0b001101: op_ori(); break;
		case 0b101011: op_sw(); break;
		case 0b001001: op_addiu(); break;
		case 0b001000: op_addi(); break;
		case 0b000010: op_j(); break;
		case 0b010000: op_cop0(); break;
		case 0b010001: op_cop1(); break;
		case 0b010010: op_cop2(); break;
		case 0b010011: op_cop3(); break;
		case 0b100011: op_lw(); break;
		case 0b000101: op_bne(); break;
		case 0b101001: op_sh(); break;
		case 0b000011: op_jal(); break;
		case 0b001100: op_andi(); break;
		case 0b101000: op_sb(); break;
		case 0b100000: op_lb(); break;
		case 0b000100: op_beq(); break;
		case 0b000111: op_bgtz(); break;
		case 0b000110: op_blez(); break;
		case 0b100100: op_lbu(); break;
		case 0b000001: op_bxx(); break;
		case 0b001010: op_slti(); break;
		case 0b001011: op_sltiu(); break;
		case 0b100101: op_lhu(); break;
		case 0b100001: op_lh(); break;
		case 0b001110: op_xori(); break;
		case 0b110000: op_lwc0(); break;
		case 0b110001: op_lwc1(); break;
		case 0b110010: op_lwc2(); break;
		case 0b110011: op_lwc3(); break;
		case 0b111000: op_swc0(); break;
		case 0b111001: op_swc1(); break;
		case 0b111010: op_swc2(); break;
		case 0b101110: op_swr(); break;
		case 0b111011: op_swc3(); break;
		case 0b101010: op_swl(); break;
		case 0b100010: op_lwl(); break;
		case 0b100110: op_lwr(); break;
		default: op_illegal(); break;
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
	should_branch = true;
}

void CPU::exception(ExceptionType type)
{
	uint32_t handler = 0;
	switch (cop0.sr.BEV) {
		case true: handler = 0xbfc00180; break;
		case false: handler = 0x80000080; break;
	}

	uint32_t mode = cop0.sr.raw & 0x3f; // Get mode
	cop0.sr.raw &= ~0x3f; // Clear mode bits
	cop0.sr.raw |= (mode << 2) & 0x3f;

	cop0.cause.raw = (uint32_t)type << 2;
	cop0.epc = current_pc;

	if (delay_slot) {
		cop0.epc -= 4;
		set_bit(cop0.cause.raw, 31, true);
	}

	pc = handler; 
	next_pc = handler + 4;
}

void CPU::op_lui()
{
	auto data = instr.imm();
	auto rt = instr.rt();

	uint32_t val = data << 16;
	set_reg(rt, val);
}

void CPU::op_ori()
{
	auto data = instr.imm();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t val = get_reg(rs) | data;
	set_reg(rt, val);
}

void CPU::op_sw()
{
	auto data = instr.simm();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t addr = get_reg(rs) + data;
	
	if (addr % 4 == 0)
		write<uint32_t>(addr, get_reg(rt));
	else
		exception(WriteError);
}

void CPU::op_lw()
{
	auto data = instr.simm();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t s = get_reg(rs);
	uint32_t addr = s + data;
	
	if (addr % 4 == 0)
		load = std::make_pair(rt, read(addr));
	else
		exception(ReadError);
}

void CPU::op_sll()
{
	auto data = instr.shift();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t val = get_reg(rt) << data;
	set_reg(rd, val);
}

void CPU::op_addiu()
{
	auto data = instr.simm();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t val = get_reg(rs) + data;
	set_reg(rt, val);
}

void CPU::op_addi()
{
	int32_t data = instr.simm();
	auto rt = instr.rt();
	auto rs = instr.rs();

	int32_t s = get_reg(rs);
	int64_t sum = (int64_t)s + data;
	if (sum < INT_MIN || sum > INT_MAX) exception(Overflow);

	set_reg(rt, (uint32_t)sum);
}

void CPU::op_j()
{
	uint32_t addr = instr.imm_jump();
	next_pc = (pc & 0xf0000000) | (addr << 2);
	
	should_branch = true;
}

void CPU::op_or()
{
	auto rs = instr.rs();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t val = get_reg(rs) | get_reg(rt);
	set_reg(rd, val);
}

void CPU::op_cop0()
{
	switch (instr.cop_op()) {
		case 0b00000: op_mfc0(); 
			break;
		case 0b00100: op_mtc0(); 
			break;
		case 0b10000: op_rfe(); 
			break;
		default: 
			panic("Cop0 unhanled opcode: 0x", instr.cop_op());
	}
}

void CPU::op_mfc0()
{
	auto cpu_r = instr.rt();
	auto cop_r = instr.rd().index;
	
	uint32_t val = cop0.regs[cop_r];
	load = std::make_pair(cpu_r, val);
}

void CPU::op_mtc0()
{
	auto cpu_r = instr.rt();
	auto cop_r = instr.rd().index;

	uint32_t val = get_reg(cpu_r);
	cop0.regs[cop_r] = val;
}

void CPU::op_cop1()
{
	exception(CopError);
}

void CPU::op_cop2()
{
	panic("Unhandled GTE instruction: ", instr.opcode);
}

void CPU::op_cop3()
{
	exception(CopError);
}

void CPU::op_bne()
{
	auto offset = instr.simm();
	auto rs = instr.rs();
	auto rt = instr.rt();

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
	auto data = instr.simm();
	auto rt = instr.rt();
	auto base = instr.rs();

	uint32_t addr = get_reg(base) + data;
	
	if (addr % 2 == 0)
		load = std::make_pair(rt, (uint32_t)read<uint16_t>(addr));
	else
		exception(ReadError);
}

void CPU::op_lh()
{
	auto offset = instr.simm();
	auto base = instr.rs();
	auto rt = instr.rt();

	uint32_t addr = get_reg(base) + offset;
	int16_t val = static_cast<int16_t>(read<uint16_t>(addr));
	if (addr % 2 == 0)
		load = std::make_pair(rt, (uint32_t)val);
	else
		exception(ReadError);
}

void CPU::op_lwl()
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
	auto data = instr.simm();
	auto rs = instr.rs();
	auto rt = instr.rt();

	uint32_t addr = get_reg(rs) + data;
	uint32_t cur_v = out_regs[rt.index];

	uint32_t aligned_addr = addr & ~3;
	uint32_t aligned_word = read<uint32_t>(aligned_addr);

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
	auto data = instr.simm();
	auto rs = instr.rs();
	auto rt = instr.rt();

	uint32_t addr = get_reg(rs) + data;
	uint32_t val = get_reg(rt);

	uint32_t aligned_addr = addr & ~3;
	uint32_t cur_mem = read<uint32_t>(aligned_addr);

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
	auto data = instr.simm();
	auto rs = instr.rs();
	auto rt = instr.rt();

	uint32_t addr = get_reg(rs) + data;
	uint32_t val = get_reg(rt);

	uint32_t aligned_addr = addr & ~3;
	uint32_t cur_mem = read<uint32_t>(aligned_addr);

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
	auto rd = instr.rd();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t val = get_reg(rs) < get_reg(rt);
	set_reg(rd, val);
}

void CPU::op_addu()
{
	auto rs = instr.rs();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t v = get_reg(rs) + get_reg(rt);
	set_reg(rd, v);
}

void CPU::op_sh()
{
	auto offset = instr.simm();
	auto base = instr.rs();
	auto rt = instr.rt();

	uint32_t addr = get_reg(base) + offset;
	uint16_t data = static_cast<uint16_t>(get_reg(rt));
	
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
	auto rt = instr.rt();
	auto rs = instr.rs();
	auto imm = instr.imm();

	uint32_t val = get_reg(rs) & imm;
	set_reg(rt, val);
}

void CPU::op_sb()
{
	auto offset = instr.simm();
	auto rt = instr.rt();
	auto base = instr.rs();

	uint32_t addr = get_reg(base) + offset;
	uint8_t data = static_cast<uint8_t>(get_reg(rt));
	write<uint8_t>(addr, data);
}

void CPU::op_jr()
{
	auto rs = instr.rs();
	next_pc = get_reg(rs);

	should_branch = true;
}

void CPU::op_lb()
{
	auto offset = instr.simm();
	auto base = instr.rs();
	auto rt = instr.rt();

	uint32_t addr = get_reg(base) + offset;
	int8_t data = (int8_t)read<uint8_t>(addr);
	load = std::make_pair(rt, data);
}

void CPU::op_beq()
{
	auto offset = instr.simm();
	auto rs = instr.rs();
	auto rt = instr.rt();

	if (get_reg(rs) == get_reg(rt))
		branch(offset);
}

void CPU::op_and()
{
	auto rs = instr.rs();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t val = get_reg(rs) & get_reg(rt);
	set_reg(rd, val);
}

void CPU::op_add()
{
	auto rs = instr.rs();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t v = get_reg(rs) + get_reg(rt);
	set_reg(rd, v);

	int32_t s = get_reg(rs);
	int32_t t = get_reg(rt);

	int64_t sum = s + t;
	if (sum < INT_MIN || sum > INT_MAX) exception(Overflow);

	set_reg(rd, (uint32_t)sum);
}

void CPU::op_bgtz()
{
	auto offset = instr.simm();
	auto rs = instr.rs();

	int32_t val = static_cast<int32_t>(get_reg(rs));
	if (val > 0)
		branch(offset);
}

void CPU::op_blez()
{
	auto offset = instr.simm();
	auto rs = instr.rs();
	
	int32_t val = static_cast<int32_t>(get_reg(rs));
	if (val <= 0)
		branch(offset);
}

void CPU::op_lbu()
{
	auto data = instr.simm();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t addr = get_reg(rs) + data;
	uint8_t val = read<uint8_t>(addr);

	load = std::make_pair(rt, (uint32_t)val);
}

void CPU::op_jalr()
{
	auto rd = instr.rd();
	auto rs = instr.rs();

	set_reg(rd, next_pc);
	next_pc = get_reg(rs);

	should_branch = true;
}

void CPU::op_bxx()
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

void CPU::op_slti()
{
	int32_t i = static_cast<int32_t>(instr.simm());
	auto rs = instr.rs();
	auto rt = instr.rt();

	auto v = static_cast<int32_t>(get_reg(rs)) < i;
	set_reg(rt, (uint32_t)v);
}

void CPU::op_subu()
{
	auto rd = instr.rd();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t val = get_reg(rs) - get_reg(rt);
	set_reg(rd, val);
}

void CPU::op_sra()
{
	auto i = instr.shift();
	auto rd = instr.rd();
	auto rt = instr.rt();

	auto  val = (int32_t)get_reg(rt) >> i;
	set_reg(rd, (uint32_t)val);
}

void CPU::op_div()
{
	auto rs = instr.rs();
	auto rt = instr.rt();

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
	auto rd = instr.rd();
	set_reg(rd, lo);
}

void CPU::op_nor()
{
	auto rs = instr.rs();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t val = !(get_reg(rs) | get_reg(rt));
	set_reg(rd, val);
}

void CPU::op_slt()
{
	auto rd = instr.rd();
	auto rs = instr.rs();
	auto rt = instr.rt();

	int32_t t = static_cast<int32_t>(get_reg(rt));
	int32_t s = static_cast<int32_t>(get_reg(rs));

	uint32_t val = s < t;
	set_reg(rd, val);
}

void CPU::op_sltiu()
{
	auto data = instr.simm();
	auto rs = instr.rs();
	auto rt = instr.rt();

	uint32_t val = get_reg(rs) < data;
	set_reg(rt, val);
}

void CPU::op_sub()
{
	auto rd = instr.rd();
	auto rt = instr.rt();
	auto rs = instr.rs();

	int64_t val = (int64_t)get_reg(rs) - (int64_t)get_reg(rt);
	if (val < INT_MIN || val > INT_MAX) exception(Overflow);
	set_reg(rd, (uint32_t)val);
}

void CPU::op_xor()
{
	auto rs = instr.rs();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t val = get_reg(rs) ^ get_reg(rt);
	set_reg(rd, val);
}

void CPU::op_xori()
{
	auto rs = instr.rs();
	auto rt = instr.rt();
	auto data = instr.imm();

	uint32_t val = get_reg(rs) ^ data;
	set_reg(rt, val);
}

void CPU::op_sllv()
{
	auto rd = instr.rd();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t val = get_reg(rt) << (get_reg(rs) & 0x1f);
	set_reg(rd, val);
}

void CPU::op_srav()
{
	auto rd = instr.rd();
	auto rt = instr.rt();
	auto rs = instr.rs();
	
	int32_t val = (int32_t)get_reg(rt) >> (get_reg(rs) & 0x1f);
	set_reg(rd, (uint32_t)val);
}

void CPU::op_srl()
{
	auto i = instr.shift();
	auto rt = instr.rt();
	auto rd = instr.rd();

	uint32_t val = get_reg(rt) >> i;
	set_reg(rd, val);
}

void CPU::op_srlv()
{
	auto rd = instr.rd();
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint32_t val = get_reg(rt) >> (get_reg(rs) & 0x1f);
	set_reg(rd, val);
}

void CPU::op_divu()
{
	auto s = instr.rs();
	auto t = instr.rt();
	
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
	auto rd = instr.rd();
	set_reg(rd, hi);
}

void CPU::op_mflo()
{
	auto rd = instr.rd();
	set_reg(rd, lo);
}

void CPU::op_mthi()
{
	auto rs = instr.rs();
	hi = get_reg(rs);
}

void CPU::op_mtlo()
{
	auto rs = instr.rs();
	lo = get_reg(rs);
}

void CPU::op_mult()
{
	auto rt = instr.rt();
	auto rs = instr.rs();

	int64_t v1 = (int32_t)get_reg(rs);
	int64_t v2 = (int32_t)get_reg(rt);
	uint64_t result = v1 * v2;
	
	hi = result >> 32;
	lo = result;
}

void CPU::op_multu()
{
	auto rt = instr.rt();
	auto rs = instr.rs();

	uint64_t v1 = get_reg(rs);
	uint64_t v2 = get_reg(rt);
	uint64_t result = v1 * v2;
	
	hi = result >> 32;
	lo = result;
}