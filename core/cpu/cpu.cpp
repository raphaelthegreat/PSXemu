#include <stdafx.hpp>
#include "cpu.h"

CPU::CPU(Bus* bus) :
    gte(this)
{
    this->bus = bus;

    /*Set PRID Processor ID*/
    cop0.PRId = 0x2;

    /* Fill opcode table. */
    register_opcodes();

    /* Reset CPU state. */
    reset();
}

CPU::~CPU()
{
    if (log_file.is_open()) log_file.close();
}

void CPU::tick()
{
    /* Fetch next instruction. */
    fetch();

    //if (pc == 0x8002dde4) __debugbreak();

    if (should_log) {
        if (!log_file.is_open()) log_file.open("log.txt");

        log_file << "PC: 0x" << std::hex << pc << '\n';
    }

    /* Execute it. */
    auto& handler = lookup[instr.opcode()];
    if (handler != nullptr)
        handler();
    else
        exception(ExceptionType::IllegalInstr);

    /* Apply pending load delays. */
    handle_load_delay();

    //force_test();
}

void CPU::force_test()
{
    if (pc == 0x80030000 && exe) {
        PSEXELoadInfo psxexe_load_info;
        if (bus->loadEXE("./tests/timers/timers.exe", psxexe_load_info)) {
            pc = psxexe_load_info.pc;
            next_pc = pc + 4;

            printf("[CPU] Sideloading exe file.\n");

            registers[28] = psxexe_load_info.r28;
            
            if (psxexe_load_info.r29 != 0) {
                registers[29] = psxexe_load_info.r29;
                registers[30] = psxexe_load_info.r30;
            }

            exe = false;
        }
    }
}

void CPU::break_on_next_tick()
{
    should_break = true;
}

void CPU::reset()
{
    /* Reset registers and PC. */
    pc = 0xbfc00000;
    next_pc = pc + 4;
    current_pc = 0;
    hi = 0; lo = 0;

    /* Clear general purpose registers. */
    memset(registers, 0, 32 * sizeof(uint));

    /* Clear CPU state. */
    is_branch = false;
    is_delay_slot = false;
    took_branch = false;
    in_delay_slot_took_branch = false;
}

void CPU::handle_interrupts()
{
    uint load = read(pc);
    uint instr = load >> 26;

    /* If it is a GTE instruction do not execute interrupt! */
    if (instr == 0x12) {
        return;
    }

    /* Update external irq bit in CAUSE register. */
    /* This bit is set when an interrupt is pending. */
    bool pending = (i_stat & i_mask) != 0;
    cop0.cause.IP = util::set_bit(cop0.cause.IP, 0, pending);

    /* Get interrupt state from Cop0. */
    bool irq_enabled = cop0.sr.IEc;

    uint irq_mask = (cop0.sr.raw >> 8) & 0xFF;
    uint irq_pending = (cop0.cause.raw >> 8) & 0xFF;

    /* If pending and enabled, handle the interrupt. */
    if (irq_enabled && (irq_mask & irq_pending) > 0) {
        exception(ExceptionType::Interrupt);
    }
}

void CPU::fetch()
{
    instr.value = read(pc);

    /* Update PC. */
    current_pc = pc;
    pc = next_pc;
    next_pc += 4;

    /* Update (load) delay slots. */
    is_delay_slot = is_branch;
    in_delay_slot_took_branch = took_branch;
    is_branch = false;
    took_branch = false;

    /* Check aligment errors. */
    if ((current_pc % 4) != 0) {
        cop0.BadA = current_pc;

        exception(ExceptionType::ReadError);
        return;
    }
}

uint CPU::read_irq(uint address)
{
    uint offset = INTERRUPT.offset(address);

    if (offset == 0)
        return i_stat;
    else if (offset == 4)
        return i_mask;
    else
        return 0;
}

void CPU::write_irq(uint address, uint value)
{
    uint offset = INTERRUPT.offset(address);

    if (offset == 0)
        i_stat &= value;
    else if (offset == 4)
        i_mask = value & 0x7FF;
    else
        return;
}

void CPU::trigger(Interrupt interrupt)
{
    i_stat |= (1 << (uint)interrupt);
}

void CPU::op_mfc2()
{
    load(instr.rt(), gte.read_data(instr.rd()));
}

void CPU::op_mtc2()
{
    gte.write_data(instr.rd(), registers[instr.rt()]);
}

void CPU::op_cfc2()
{
    load(instr.rt(), gte.read_control(instr.rd()));
}

void CPU::op_ctc2()
{
    gte.write_control(instr.rd(), registers[instr.rt()]);
}

void CPU::exception(ExceptionType cause, uint cop)
{
    uint mode = cop0.sr.raw & 0x3F;
    cop0.sr.raw &= ~(uint)0x3F;
    cop0.sr.raw |= (mode << 2) & 0x3F;

    uint copy = cop0.cause.raw & 0xff00;
    cop0.cause.exc_code = (uint)cause;
    cop0.cause.CE = cop;

    if (cause == ExceptionType::Interrupt) {
        cop0.epc = pc;

        /* Hack: related to the delay of the ex interrupt*/
        is_delay_slot = is_branch;
        in_delay_slot_took_branch = took_branch;
    }
    else {
        cop0.epc = current_pc;
    }

    if (is_delay_slot) {
        cop0.epc -= 4;

        cop0.cause.BD = true;
        cop0.TAR = pc;

        if (in_delay_slot_took_branch) {
            cop0.cause.BT = true;
        }
    }

    /* Select exception address. */
    pc = exception_addr[cop0.sr.BEV];
    next_pc = pc + 4;
}

void CPU::handle_load_delay()
{
    if (delayed_memory_load.reg != memory_load.reg) {
        registers[memory_load.reg] = memory_load.value;
    }
    memory_load = delayed_memory_load;
    delayed_memory_load.reg = 0;

    registers[write_back.reg] = write_back.value;
    write_back.reg = 0;
    registers[0] = 0;
}

void CPU::op_bcond()
{
    is_branch = true;
    uint op = instr.rt();

    bool should_link = (op & 0x1E) == 0x10;
    bool should_branch = (int)(registers[instr.rs()] ^ (op << 31)) < 0;

    if (should_link) registers[31] = next_pc;
    if (should_branch) branch();
}

void CPU::op_swr()
{
    uint addr = registers[instr.rs()] + instr.imm_s();
    uint aligned_addr = addr & 0xFFFFFFFC;
    uint aligned_load = bus->read(aligned_addr);

    uint value = 0;
    switch (addr & 0b11) {
    case 0:
        value = registers[instr.rt()]; break;
    case 1:
        value = (aligned_load & 0x000000FF) | (registers[instr.rt()] << 8);
        break;
    case 2:
        value = (aligned_load & 0x0000FFFF) | (registers[instr.rt()] << 16);
        break;
    case 3:
        value = (aligned_load & 0x00FFFFFF) | (registers[instr.rt()] << 24);
        break;
    }

    bus->write(aligned_addr, value);
}

void CPU::op_swl()
{
    uint addr = registers[instr.rs()] + instr.imm_s();
    uint aligned_addr = addr & 0xFFFFFFFC;
    uint aligned_load = bus->read(aligned_addr);

    uint value = 0;
    switch (addr & 0b11) {
    case 0:
        value = (aligned_load & 0xFFFFFF00) | (registers[instr.rt()] >> 24);
        break;
    case 1:
        value = (aligned_load & 0xFFFF0000) | (registers[instr.rt()] >> 16);
        break;
    case 2:
        value = (aligned_load & 0xFF000000) | (registers[instr.rt()] >> 8);
        break;
    case 3:
        value = registers[instr.rt()]; break;
    }

    bus->write(aligned_addr, value);
}

void CPU::op_lwr()
{
    uint addr = registers[instr.rs()] + instr.imm_s();
    uint aligned_addr = addr & 0xFFFFFFFC;
    uint aligned_load = bus->read(aligned_addr);

    uint value = 0;
    uint LRValue = registers[instr.rt()];

    if (instr.rt() == memory_load.reg) {
        LRValue = memory_load.value;
    }

    switch (addr & 0b11) {
    case 0:
        value = aligned_load;
        break;
    case 1:
        value = (LRValue & 0xFF000000) | (aligned_load >> 8);
        break;
    case 2:
        value = (LRValue & 0xFFFF0000) | (aligned_load >> 16);
        break;
    case 3:
        value = (LRValue & 0xFFFFFF00) | (aligned_load >> 24);
        break;
    }

    load(instr.rt(), value);
}

void CPU::op_lwl()
{
    uint addr = registers[instr.rs()] + instr.imm_s();
    uint aligned_addr = addr & 0xFFFFFFFC;
    uint aligned_load = bus->read(aligned_addr);

    uint value = 0;
    uint LRValue = registers[instr.rt()];

    if (instr.rt() == memory_load.reg) {
        LRValue = memory_load.value;
    }

    switch (addr & 0b11) {
    case 0:
        value = (LRValue & 0x00FFFFFF) | (aligned_load << 24);
        break;
    case 1:
        value = (LRValue & 0x0000FFFF) | (aligned_load << 16);
        break;
    case 2:
        value = (LRValue & 0x000000FF) | (aligned_load << 8);
        break;
    case 3:
        value = aligned_load;
        break;
    }

    load(instr.rt(), value);
}

void CPU::op_xori()
{
    set_reg(instr.rt(), registers[instr.rs()] ^ instr.imm());
}

void CPU::op_sub()
{
    uint rs = registers[instr.rs()];
    uint rt = registers[instr.rt()];
    uint sub = rs - rt;

    bool overflow = util::sub_overflow(rs, rt, sub);
    if (!overflow)
        set_reg(instr.rd(), sub);
    else
        exception(ExceptionType::Overflow, instr.id());
}

void CPU::op_mult()
{
    int64_t value = (int64_t)(int)registers[instr.rs()] * (int64_t)(int)registers[instr.rt()]; //sign extend to pass amidog cpu test

    hi = (uint)(value >> 32);
    lo = (uint)value;
}

void CPU::op_break()
{
    exception(ExceptionType::Break);
}

void CPU::op_xor()
{
    set_reg(instr.rd(), registers[instr.rs()] ^ registers[instr.rt()]);
}

void CPU::op_multu()
{
    ulong value = (ulong)registers[instr.rs()] * (ulong)registers[instr.rt()]; //sign extend to pass amidog cpu test

    hi = (uint)(value >> 32);
    lo = (uint)value;
}

void CPU::op_srlv()
{
    set_reg(instr.rd(), registers[instr.rt()] >> (int)(registers[instr.rs()] & 0x1F));
}

void CPU::op_srav()
{
    set_reg(instr.rd(), (uint)((int)registers[instr.rt()] >> (int)(registers[instr.rs()] & 0x1F)));
}

void CPU::op_nor()
{
    set_reg(instr.rd(), ~(registers[instr.rs()] | registers[instr.rt()]));
}

void CPU::op_lh()
{
    if (!cop0.sr.IsC) {
        uint addr = registers[instr.rs()] + instr.imm_s();

        if ((addr & 0x1) == 0) {
            uint value = (uint)(short)bus->read<ushort>(addr);
            load(instr.rt(), value);
        }
        else {
            cop0.BadA = addr;
            exception(ExceptionType::ReadError, instr.id());
        }
    }
}

void CPU::op_sllv()
{
    set_reg(instr.rd(), registers[instr.rt()] << (int)(registers[instr.rs()] & 0x1F));
}

void CPU::op_lhu()
{
    if (!cop0.sr.IsC) {
        uint addr = registers[instr.rs()] + instr.imm_s();

        if ((addr & 0x1) == 0) {
            uint value = bus->read<ushort>(addr);
            load(instr.rt(), value);
        }
        else {
            cop0.BadA = addr;
            exception(ExceptionType::ReadError, instr.id());
        }
    }
}

void CPU::op_rfe()
{
    uint mode = cop0.sr.raw & 0x3F;

    /* Shift kernel/user mode bits back. */
    cop0.sr.raw &= ~(uint)0xF;
    cop0.sr.raw |= mode >> 2;
}

void CPU::op_mthi()
{
    hi = registers[instr.rs()];
}

void CPU::op_mtlo()
{
    lo = registers[instr.rs()];
}

void CPU::op_syscall()
{
    exception(ExceptionType::Syscall, instr.id());
}

void CPU::op_slt()
{
    bool condition = (int)registers[instr.rs()] < (int)registers[instr.rt()];
    set_reg(instr.rd(), condition ? 1u : 0u);
}

void CPU::op_mfhi()
{
    set_reg(instr.rd(), hi);
}

void CPU::op_divu()
{
    uint n = registers[instr.rs()];
    uint d = registers[instr.rt()];

    if (d == 0) {
        hi = n;
        lo = 0xFFFFFFFF;
    }
    else {
        hi = n % d;
        lo = n / d;
    }
}

void CPU::op_sltiu()
{
    bool condition = registers[instr.rs()] < instr.imm_s();
    set_reg(instr.rt(), condition ? 1u : 0u);
}

void CPU::op_srl()
{
    set_reg(instr.rd(), registers[instr.rt()] >> (int)instr.sa());
}

void CPU::op_mflo()
{
    set_reg(instr.rd(), lo);
}

void CPU::op_div()
{
    int n = (int)registers[instr.rs()];
    int d = (int)registers[instr.rt()];

    if (d == 0) {
        hi = (uint)n;
        if (n >= 0) {
            lo = 0xFFFFFFFF;
        }
        else {
            lo = 1;
        }
    }
    else if ((uint)n == 0x80000000 && d == -1) {
        hi = 0;
        lo = 0x80000000;
    }
    else {
        hi = (uint)(n % d);
        lo = (uint)(n / d);
    }
}

void CPU::op_sra()
{
    set_reg(instr.rd(), (uint)((int)registers[instr.rt()] >> (int)instr.sa()));
}

void CPU::op_subu()
{
    set_reg(instr.rd(), registers[instr.rs()] - registers[instr.rt()]);
}

void CPU::op_slti()
{
    bool condition = (int)registers[instr.rs()] < (int)instr.imm_s();
    set_reg(instr.rt(), condition ? 1u : 0u);
}

void CPU::branch()
{
    took_branch = true;
    next_pc = pc + (instr.imm_s() << 2);
}

void CPU::op_jalr()
{
    set_reg(instr.rd(), next_pc);
    op_jr();
}

void CPU::op_lbu()
{
    if (!cop0.sr.IsC) {
        uint value = bus->read<ubyte>(registers[instr.rs()] + instr.imm_s());
        load(instr.rt(), value);
    }
}

void CPU::op_blez()
{
    is_branch = true;
    if (((int)registers[instr.rs()]) <= 0) {
        branch();
    }
}

void CPU::op_bgtz()
{
    is_branch = true;
    if (((int)registers[instr.rs()]) > 0) {
        branch();
    }
}

void CPU::op_add()
{
    uint rs = registers[instr.rs()];
    uint rt = registers[instr.rt()];
    uint add = rs + rt;

    bool overflow = util::add_overflow(rs, rt, add);
    if (!overflow)
        set_reg(instr.rd(), add);
    else
        exception(ExceptionType::Overflow, instr.id());
}

void CPU::op_and()
{
    set_reg(instr.rd(), registers[instr.rs()] & registers[instr.rt()]);
}

void CPU::op_mfc0()
{
    uint mfc = instr.rd();

    if (mfc == 3 || mfc >= 5 && mfc <= 9 || mfc >= 11 && mfc <= 15) {
        load(instr.rt(), cop0.regs[mfc]);
    }
    else {
        exception(ExceptionType::IllegalInstr, instr.id());
    }
}

void CPU::op_lwc2()
{
    uint addr = registers[instr.rs()] + instr.imm_s();

    if ((addr & 0x3) == 0) {
        uint data = read(addr);
        gte.write_data(instr.rt(), data);
    }
    else {
        cop0.BadA = addr;
        exception(ExceptionType::CoprocessorError, instr.id());
    }
}

void CPU::op_swc2()
{
    uint addr = registers[instr.rs()] + instr.imm_s();

    if ((addr & 0x3) == 0) {
        bus->write(addr, gte.read_data(instr.rt()));
    }
    else {
        cop0.BadA = addr;
        exception(ExceptionType::CoprocessorError, instr.id());
    }
}

void CPU::op_beq()
{
    is_branch = true;
    if (registers[instr.rs()] == registers[instr.rt()]) {
        branch();
    }
}

void CPU::op_lb()
{
    if (!cop0.sr.IsC) {
        uint value = (uint)(byte)bus->read<ubyte>(registers[instr.rs()] + instr.imm_s());
        load(instr.rt(), value);
    }
}

void CPU::op_jr()
{
    is_branch = true;
    took_branch = true;
    next_pc = registers[instr.rs()];
}

void CPU::op_sb()
{
    if (!cop0.sr.IsC)
        bus->write<ubyte>(registers[instr.rs()] + instr.imm_s(), (ubyte)registers[instr.rt()]);
}

void CPU::op_andi()
{
    set_reg(instr.rt(), registers[instr.rs()] & instr.imm());
}

void CPU::op_jal()
{
    set_reg(31, next_pc);
    op_j();
}

void CPU::op_sh()
{
    if (!cop0.sr.IsC) {
        uint addr = registers[instr.rs()] + instr.imm_s();

        if ((addr & 0x1) == 0) {
            bus->write<ushort>(addr, (ushort)registers[instr.rt()]);
        }
        else {
            cop0.BadA = addr;
            exception(ExceptionType::WriteError, instr.id());
        }
    }
}

void CPU::op_addu()
{
    set_reg(instr.rd(), registers[instr.rs()] + registers[instr.rt()]);
}

void CPU::op_sltu()
{
    bool condition = registers[instr.rs()] < registers[instr.rt()];
    set_reg(instr.rd(), condition ? 1u : 0u);
}

void CPU::op_lw()
{
    if (!cop0.sr.IsC) {
        uint addr = registers[instr.rs()] + instr.imm_s();

        if ((addr & 0x3) == 0) {
            uint value = bus->read(addr);
            load(instr.rt(), value);
        }
        else {
            cop0.BadA = addr;
            exception(ExceptionType::ReadError, instr.id());
        }

    }
}

void CPU::op_addi()
{
    uint rs = registers[instr.rs()];
    uint imm_s = instr.imm_s();
    uint addi = rs + imm_s;

    bool overflow = util::add_overflow(rs, imm_s, addi);
    if (!overflow)
        set_reg(instr.rt(), addi);
    else
        exception(ExceptionType::Overflow, instr.id());
}
// TODO
void CPU::op_bne()
{
    uint rs = instr.rs();
    uint rt = instr.rt();

    is_branch = true;
    if (registers[rs] != registers[rt]) {
        branch();
    }
}

void CPU::op_mtc0()
{
    uint value = registers[instr.rt()];
    uint reg = instr.rd();

    bool prev_IEC = cop0.sr.IEc;

    if (reg == 13) {
        cop0.cause.raw &= ~(uint)0x300;
        cop0.cause.raw |= value & 0x300;
    }
    else {
        cop0.regs[reg] = value;
    }

    uint irq_mask = cop0.sr.Sw | (cop0.sr.Intr >> 2);
    uint irq_pending = cop0.cause.Sw | (cop0.cause.IP >> 2);

    if (!prev_IEC && cop0.sr.IEc && (irq_mask & irq_pending) > 0) {
        pc = next_pc;
        exception(ExceptionType::Interrupt, instr.id());
    }
}

void CPU::op_or()
{
    set_reg(instr.rd(), registers[instr.rs()] | registers[instr.rt()]);
}

void CPU::op_j()
{
    is_branch = true;
    took_branch = true;
    next_pc = (next_pc & 0xF0000000) | (instr.addr() << 2);
}

void CPU::op_addiu()
{
    uint rt = instr.rt();
    uint rs = instr.rs();
    uint imm = instr.imm_s();

    set_reg(instr.rt(), registers[instr.rs()] + instr.imm_s());
}

void CPU::op_sll()
{
    uint rd = instr.rd();
    uint rt = instr.rt();
    uint sa = instr.sa();

    set_reg(instr.rd(), registers[instr.rt()] << (int)instr.sa());
}

void CPU::op_sw()
{
    if (!cop0.sr.IsC) {
        uint r = instr.rs();
        uint i = instr.imm_s();
        uint addr = registers[r] + i;

        if ((addr & 0x3) == 0) {
            bus->write(addr, registers[instr.rt()]);
        }
        else {
            cop0.BadA = addr;
            exception(ExceptionType::WriteError, instr.id());
        }
    }
}

void CPU::op_lui()
{
    set_reg(instr.rt(), instr.imm() << 16);
}

void CPU::op_ori()
{
    set_reg(instr.rt(), registers[instr.rs()] | instr.imm());
}

void CPU::set_reg(uint regN, uint value)
{
    write_back.reg = regN;
    write_back.value = value;
}

void CPU::load(uint regN, uint value)
{
    delayed_memory_load.reg = regN;
    delayed_memory_load.value = value;
}