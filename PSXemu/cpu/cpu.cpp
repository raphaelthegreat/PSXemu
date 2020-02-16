#include "cpu.h"

CPU::CPU(Bus* bus)
{
    this->bus = bus;
    
    /*Set PRID Processor ID*/
    cop0.PRId = 0x2;
    
    /* Fill opcode table. */
    register_opcodes();
    
    /* Reset CPU state. */
    reset();
}

void CPU::tick()
{
    /* Fetch next instruction. */
    fetch();
    
    /* Execute it. */
    auto& handler = lookup[instr.opcode()];
    handler();

    /* Apply pending load delays. */
    handle_load_delay();
}

void CPU::reset()
{
    /* Reset registers and PC. */
    pc = 0xbfc00000;
    next_pc = pc + 4;
    current_pc = 0;
    hi = 0; lo = 0;

    /* Clear general purpose registers. */
    memset(registers, 0, 32 * sizeof(uint32_t));

    /* Clear CPU state. */
    is_branch = false;
    is_delay_slot = false;
    took_branch = false;
    in_delay_slot_took_branch = false;
}

void CPU::handle_interrupts()
{
    uint32_t load = bus->read(pc);
    uint32_t instr = load >> 26;
    
    /* If it is a GTE instruction do not execute interrupt! */
    if (instr == 0x12) {       
        return;
    }

    /* Update external irq bit in CAUSE register. */
    /* This bit is set when an interrupt is pending. */
    bool pending = bus->interruptController.interruptPending();
    set_bit(cop0.cause.IP, 0, pending);

    /* Get interrupt state from Cop0. */
    bool irq_enabled = cop0.sr.IEc;

    uint32_t irq_mask = cop0.sr.Sw | (cop0.sr.Intr >> 2);
    uint32_t irq_pending = cop0.cause.Sw | (cop0.cause.IP >> 2);

    /* If pending and enabled, handle the interrupt. */
    if (irq_enabled && (irq_mask & irq_pending) > 0) {
        exception(ExceptionType::Interrupt);
    }
}

void CPU::fetch()
{
    uint32_t load = bus->read(pc);
    
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
    if (current_pc % 4 != 0) {
        cop0.BadA = current_pc;
        
        exception(ExceptionType::ReadError);
        return;
    }

    instr.Decode(load);
}

void CPU::exception(ExceptionType cause, uint32_t cop)
{
    uint32_t mode = cop0.sr.raw & 0x3F;
    cop0.sr.raw &= ~(uint32_t)0x3F;
    cop0.sr.raw |= (mode << 2) & 0x3F;

    uint32_t copy = cop0.cause.raw & 0xff00;
    cop0.cause.exc_code = (uint32_t)cause;
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
    if (delayed_memory_load.reg != memory_load.reg) { //if loadDelay on same reg it is lost/overwritten (amidog tests)
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
    uint32_t op = instr.rt();

    bool should_link = (op & 0x1E) == 0x10;
    bool should_branch = (int)(registers[instr.rs()] ^ (op << 31)) < 0;

    if (should_link) registers[31] = next_pc;
    if (should_branch) branch();
}

void CPU::op_swr()
{
    uint32_t addr = registers[instr.rs()] + instr.imm_s();
    uint32_t aligned_addr = addr & 0xFFFFFFFC;
    uint32_t aligned_load = bus->read(aligned_addr);

    uint32_t value = 0;
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
    uint32_t addr = registers[instr.rs()] + instr.imm_s();
    uint32_t aligned_addr = addr & 0xFFFFFFFC;
    uint32_t aligned_load = bus->read(aligned_addr);

    uint32_t value = 0;
    switch (addr & 0b11) {
    case 0: value = (aligned_load & 0xFFFFFF00) | (registers[instr.rt()] >> 24); break;
    case 1: value = (aligned_load & 0xFFFF0000) | (registers[instr.rt()] >> 16); break;
    case 2: value = (aligned_load & 0xFF000000) | (registers[instr.rt()] >> 8); break;
    case 3: value = registers[instr.rt()]; break;
    }

    bus->write(aligned_addr, value);
}

void CPU::op_lwr()
{
    uint32_t addr = registers[instr.rs()] + instr.imm_s();
    uint32_t aligned_addr = addr & 0xFFFFFFFC;
    uint32_t aligned_load = bus->read(aligned_addr);

    uint32_t value = 0;
    uint32_t LRValue = registers[instr.rt()];

    if (instr.rt() == memory_load.reg) {
        LRValue = memory_load.value;
    }

    switch (addr & 0b11) {
    case 0: value = aligned_load; break;
    case 1: value = (LRValue & 0xFF000000) | (aligned_load >> 8); break;
    case 2: value = (LRValue & 0xFFFF0000) | (aligned_load >> 16); break;
    case 3: value = (LRValue & 0xFFFFFF00) | (aligned_load >> 24); break;
    }

    delayedLoad(instr.rt(), value);
}

void CPU::op_lwl()
{
    uint32_t addr = registers[instr.rs()] + instr.imm_s();
    uint32_t aligned_addr = addr & 0xFFFFFFFC;
    uint32_t aligned_load = bus->read(aligned_addr);

    uint32_t value = 0;
    uint32_t LRValue = registers[instr.rt()];

    if (instr.rt() == memory_load.reg) {
        LRValue = memory_load.value;
    }

    switch (addr & 0b11) {
    case 0: value = (LRValue & 0x00FFFFFF) | (aligned_load << 24); break;
    case 1: value = (LRValue & 0x0000FFFF) | (aligned_load << 16); break;
    case 2: value = (LRValue & 0x000000FF) | (aligned_load << 8); break;
    case 3: value = aligned_load; break;
    }
    //Console.WriteLine("case " + (addr & 0b11) + " LWL Value " + value.ToString("x8"));
    delayedLoad(instr.rt(), value);
}

void CPU::op_xori()
{
    setregisters(instr.rt(), registers[instr.rs()] ^ instr.imm());
}

void CPU::op_sub()
{
    int rs = (int)registers[instr.rs()];
    int rt = (int)registers[instr.rt()];

    uint32_t sub = 0;
    bool overflow = !safe_add(&sub, rs, -rt);

    if (!overflow) {
        setregisters(instr.rd(), sub);
    }
    else {
        exception(ExceptionType::Overflow, instr.id());
    }
}

void CPU::op_mult()
{
    int64_t value = (int64_t)(int)registers[instr.rs()] * (int64_t)(int)registers[instr.rt()]; //sign extend to pass amidog cpu test

    hi = (uint32_t)(value >> 32);
    lo = (uint32_t)value;
}

void CPU::op_break()
{
    exception(ExceptionType::Break);
}

void CPU::op_xor()
{
    setregisters(instr.rd(), registers[instr.rs()] ^ registers[instr.rt()]);
}

void CPU::op_multu()
{
    uint64_t value = (uint64_t)registers[instr.rs()] * (uint64_t)registers[instr.rt()]; //sign extend to pass amidog cpu test

    hi = (uint32_t)(value >> 32);
    lo = (uint32_t)value;
}

void CPU::op_srlv()
{
    setregisters(instr.rd(), registers[instr.rt()] >> (int)(registers[instr.rs()] & 0x1F));
}

void CPU::op_srav()
{
    setregisters(instr.rd(), (uint32_t)((int)registers[instr.rt()] >> (int)(registers[instr.rs()] & 0x1F)));
}

void CPU::op_nor()
{
    setregisters(instr.rd(), ~(registers[instr.rs()] | registers[instr.rt()]));
}

void CPU::op_lh()
{
    if (!cop0.sr.IsC) {
        uint32_t addr = registers[instr.rs()] + instr.imm_s();

        if ((addr & 0x1) == 0) {
            uint32_t value = (uint32_t)(short)bus->read<uint16_t>(addr);
            delayedLoad(instr.rt(), value);
        }
        else {
            cop0.BadA = addr;
            exception(ExceptionType::ReadError, instr.id());
        }

    }
}

void CPU::op_sllv()
{
    setregisters(instr.rd(), registers[instr.rt()] << (int)(registers[instr.rs()] & 0x1F));
}

void CPU::op_lhu()
{
    if (!cop0.sr.IsC) {
        uint32_t addr = registers[instr.rs()] + instr.imm_s();

        if ((addr & 0x1) == 0) {
            uint32_t value = bus->read<uint16_t>(addr);
            //Console.WriteLine("LHU: " + addr.ToString("x8") + " value: " + value.ToString("x8"));
            delayedLoad(instr.rt(), value);
        }
        else {
            cop0.BadA = addr;
            exception(ExceptionType::ReadError, instr.id());
        }

    } //else Console.WriteLine("Ignoring Load");
}

void CPU::op_rfe()
{
    uint32_t mode = cop0.sr.raw & 0x3F;
    
    /* Shift kernel/user mode bits back. */
    cop0.sr.raw &= ~(uint32_t)0xF;
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
    setregisters(instr.rd(), condition ? 1u : 0u);
}

void CPU::op_mfhi()
{
    setregisters(instr.rd(), hi);
}

void CPU::op_divu()
{
    uint32_t n = registers[instr.rs()];
    uint32_t d = registers[instr.rt()];

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
    setregisters(instr.rt(), condition ? 1u : 0u);
}

void CPU::op_srl()
{
    setregisters(instr.rd(), registers[instr.rt()] >> (int)instr.sa());
}

void CPU::op_mflo()
{
    setregisters(instr.rd(), lo);
}

void CPU::op_div()
{
    int n = (int)registers[instr.rs()];
    int d = (int)registers[instr.rt()];

    if (d == 0) {
        hi = (uint32_t)n;
        if (n >= 0) {
            lo = 0xFFFFFFFF;
        }
        else {
            lo = 1;
        }
    }
    else if ((uint32_t)n == 0x80000000 && d == -1) {
        hi = 0;
        lo = 0x80000000;
    }
    else {
        hi = (uint32_t)(n % d);
        lo = (uint32_t)(n / d);
    }
}

void CPU::op_sra()
{
    setregisters(instr.rd(), (uint32_t)((int)registers[instr.rt()] >> (int)instr.sa()));
}

void CPU::op_subu()
{
    setregisters(instr.rd(), registers[instr.rs()] - registers[instr.rt()]);
}

void CPU::op_slti()
{
    bool condition = (int)registers[instr.rs()] < (int)instr.imm_s();
    setregisters(instr.rt(), condition ? 1u : 0u);
}

void CPU::branch()
{
    took_branch = true;
    next_pc = pc + (instr.imm_s() << 2);
}

void CPU::op_jalr()
{
    setregisters(instr.rd(), next_pc);
    op_jr();
}

void CPU::op_lbu()
{
    if (!cop0.sr.IsC) {
        uint32_t value = bus->read<uint8_t>(registers[instr.rs()] + instr.imm_s());
        delayedLoad(instr.rt(), value);
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
    int rs = (int)registers[instr.rs()];
    int rt = (int)registers[instr.rt()];

    uint32_t add = 0;
    bool overflow = !safe_add(&add, rs, rt);

    if (!overflow) {
        setregisters(instr.rd(), add);
    }
    else {
        exception(ExceptionType::Overflow, instr.id());
    }
}

void CPU::op_and()
{
    setregisters(instr.rd(), registers[instr.rs()] & registers[instr.rt()]);
}

void CPU::op_mfc0()
{
    uint32_t mfc = instr.rd();
    
    if (mfc == 3 || mfc >= 5 && mfc <= 9 || mfc >= 11 && mfc <= 15) {
        delayedLoad(instr.rt(), cop0.regs[mfc]);
    }
    else {
        exception(ExceptionType::IllegalInstr, instr.id());
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
{ //todo redo this as it unnecesary read
    if (!cop0.sr.IsC) {
        uint32_t value = (uint32_t)(uint8_t)bus->read<uint8_t>(registers[instr.rs()] + instr.imm_s());
        delayedLoad(instr.rt(), value);
    } //else Console.WriteLine("Ignoring Write");
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
        bus->write<uint8_t>(registers[instr.rs()] + instr.imm_s(), (int8_t)registers[instr.rt()]);
}

void CPU::op_andi()
{
    setregisters(instr.rt(), registers[instr.rs()] & instr.imm());
}

void CPU::op_jal()
{
    setregisters(31, next_pc);
    op_j();
}

void CPU::op_sh()
{
    if (!cop0.sr.IsC) {
        uint32_t addr = registers[instr.rs()] + instr.imm_s();

        if ((addr & 0x1) == 0) {
            bus->write<uint16_t>(addr, (uint16_t)registers[instr.rt()]);
        }
        else {
            cop0.BadA = addr;
            exception(ExceptionType::WriteError, instr.id());
        }
    }
    //else Console.WriteLine("Ignoring Write");
}

void CPU::op_addu()
{
    setregisters(instr.rd(), registers[instr.rs()] + registers[instr.rt()]);
}

void CPU::op_sltu()
{
    bool condition = registers[instr.rs()] < registers[instr.rt()];
    setregisters(instr.rd(), condition ? 1u : 0u);
}

void CPU::op_lw()
{
    if (!cop0.sr.IsC) {
        uint32_t addr = registers[instr.rs()] + instr.imm_s();

        if ((addr & 0x3) == 0) {
            uint32_t value = bus->read(addr);
            delayedLoad(instr.rt(), value);
        }
        else {
            cop0.BadA = addr;
            exception(ExceptionType::ReadError, instr.id());
        }

    }
}

void CPU::op_addi()
{
    int rs = (int)registers[instr.rs()];
    int imm_s = (int)instr.imm_s();

    uint32_t addi = 0;
    bool overflow = !safe_add(&addi, rs, imm_s);

    if (!overflow) {
        setregisters(instr.rt(), addi);
    }
    else {
        exception(ExceptionType::Overflow, instr.id());
    }
}

void CPU::op_bne()
{
    is_branch = true;
    if (registers[instr.rs()] != registers[instr.rt()]) {
        branch();
    }
}

void CPU::op_mtc0()
{
    uint32_t value = registers[instr.rt()];
    uint32_t reg = instr.rd();

    //MTC0 can trigger soft interrupts
    bool prev_IEC = cop0.sr.IEc;

    if (reg == 13) { //only bits 8 and 9 are writable
        cop0.cause.raw &= ~(uint32_t)0x300;
        cop0.cause.raw |= value & 0x300;
    }
    else {
        cop0.regs[reg] = value; //There are some zeros on SR that shouldnt be writtable also todo: registers > 16?
    }

    uint32_t irq_mask = cop0.sr.Sw | (cop0.sr.Intr >> 2);
    uint32_t irq_pending = cop0.cause.Sw | (cop0.cause.IP >> 2);

    if (!prev_IEC && cop0.sr.IEc && (irq_mask & irq_pending) > 0) {
        pc = next_pc;
        exception(ExceptionType::Interrupt, instr.id());
    }
}

void CPU::op_or()
{
    setregisters(instr.rd(), registers[instr.rs()] | registers[instr.rt()]);
}

void CPU::op_j()
{
    is_branch = true;
    took_branch = true;
    next_pc = (next_pc & 0xF0000000) | (instr.addr() << 2);
}

void CPU::op_addiu()
{
    setregisters(instr.rt(), registers[instr.rs()] + instr.imm_s());
}

void CPU::op_sll()
{
    setregisters(instr.rd(), registers[instr.rt()] << (int)instr.sa());
}

void CPU::op_sw()
{
    if (!cop0.sr.IsC) {
        uint32_t addr = registers[instr.rs()] + instr.imm_s();

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
    setregisters(instr.rt(), instr.imm() << 16);
}

void CPU::op_ori()
{
    setregisters(instr.rt(), registers[instr.rs()] | instr.imm());
}

void CPU::setregisters(uint32_t regN, uint32_t value)
{
    write_back.reg = regN;
    write_back.value = value;
}

void CPU::delayedLoad(uint32_t regN, uint32_t value)
{
    delayed_memory_load.reg = regN;
    delayed_memory_load.value = value;
}
