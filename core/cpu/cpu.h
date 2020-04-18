#pragma once
#include <memory/bus.h>
#include <cpu/gte.h>
#include <cpu/instr.hpp>
#include <cpu/cop0.h>

struct MEM {
    uint reg = 0;
    uint value = 0;
};

typedef std::function<void()> CPUfunc;

/* Memory Ranges. */
const Range KSEG = Range(0x00000000, 2048 * 1024 * 1024LL);
const Range KSEG0 = Range(0x80000000, 512 * 1024 * 1024LL);
const Range KSEG1 = Range(0xA0000000, 512 * 1024 * 1024LL);
const Range KSEG2 = Range(0xC0000000, 1024 * 1024 * 1024LL);
const Range INTERRUPT = Range(0x1f801070, 8);

/* A class implemeting the MIPS R3000A CPU. */
class CPU {
public:
    CPU(Bus* bus);
    ~CPU();

    /* CPU functionality. */
    void tick();
    void reset();
    void fetch();
    void branch();
    void register_opcodes();
    void handle_interrupts();
    void handle_load_delay();
    void force_test();

    void break_on_next_tick();

    void exception(ExceptionType cause, uint cop = 0);
    void set_reg(uint regN, uint value);
    void load(uint regN, uint value);

    /* Bus communication. */
    template <typename T = uint>
    T read(uint addr);
    
    template <typename T = uint>
    void write(uint addr, T data);

    uint read_irq(uint address);
    void write_irq(uint address, uint value);
    void trigger(Interrupt interrupt);

    /* Opcodes. */
    void op_special(); void op_cop2(); void op_cop0();
    void op_lwc2(); void op_swc2(); void op_mfc2(); void op_mtc2();
    void op_cfc2(); void op_ctc2();

    /* Read/Write instructions. */
    void op_lhu(); void op_lh(); void op_lbu(); void op_sw();
    void op_swr(); void op_swl(); void op_lwr(); void op_lwl();
    void op_lui(); void op_sb(); void op_lw(); void op_lb();
    void op_sh();

    /* Bitwise manipulation instructions. */
    void op_and(); void op_andi(); void op_xor(); void op_xori();
    void op_nor(); void op_or(); void op_ori();

    /* Jump and branch instructions. */
    void op_bcond(); void op_jalr(); void op_blez(); void op_bgtz(); 
    void op_j(); void op_beq(); void op_jr(); void op_jal();
    void op_bne();

    /* Arithmetic instructions. */
    void op_add(); void op_addi(); void op_addu(); void op_addiu(); 
    void op_sub(); void op_subu(); void op_mult(); void op_multu();
    void op_div(); void op_divu();
  
    /* Bit shift-rotation instructions. */
    void op_srl(); void op_srlv(); void op_sra(); void op_srav();
    void op_sll(); void op_sllv(); void op_slt(); void op_slti();
    void op_sltu(); void op_sltiu();

    /* HI/LO instructions. */
    void op_mthi(); void op_mtlo(); void op_mfhi(); void op_mflo();

    /* Exception instructions. */
    void op_break(); void op_syscall(); void op_rfe();
    
    /* Coprocessor instructions. */
    void op_mfc0(); void op_mtc0();                      

public:
    Bus* bus;

    /* Registers. */
    uint current_pc, pc, next_pc;
    uint i_stat, i_mask;
    uint registers[32] = {};
    uint hi, lo;

    /* Flow control. */
    bool is_branch, is_delay_slot;
    bool took_branch;
    bool in_delay_slot_took_branch;
    
    /* Debugging. */
    bool should_break = false;
    bool should_log = false;
    bool exe = true;
    std::ofstream log_file;
    int cycle_int = 20000;
    bool flip = true;

    uint exception_addr[2] = { 0x80000080, 0xBFC00180 };
    
    /* I-Cache. */
    CacheLine instr_cache[256] = {};
    CacheControl cache_control = {};

    /* Coprocessors. */
    Cop0 cop0;
    GTE gte;

    MEM write_back, memory_load;
    MEM delayed_memory_load;

    /* Current instruction. */
    Instr instr;

    /* Opcode lookup table. */
    std::unordered_map<uint, CPUfunc> lookup, special;
};

template<typename T>
inline T CPU::read(uint addr)
{
    /* Read from main RAM or IO. */
    return bus->read<T>(addr);
}

template<typename T>
inline void CPU::write(uint addr, T data)
{
    /* Write to main RAM or IO. */
    bus->write<T>(addr, data);
}
