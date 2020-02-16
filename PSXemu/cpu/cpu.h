#pragma once
#include <memory/bus.h>
#include <unordered_map>
#include <cpu/instr.h>
#include "cop0.h"

typedef std::function<void()> CPUfunc;

struct MEM {
    uint32_t reg = 0;
    uint32_t value = 0;
};

/* A class implemeting the MIPS R3000A CPU. */
class CPU {
public:
    CPU(Bus* bus);

    /* CPU functionality */
    void tick();
    void reset();
    void fetch();
    void branch();
    void register_opcodes();
    void handle_interrupts();
    void handle_load_delay();
    
    void exception(ExceptionType cause, uint32_t cop = 0);
    void setregisters(uint32_t regN, uint32_t value);
    void delayedLoad(uint32_t regN, uint32_t value);

    /* Opcodes. */
    void op_special(); void op_cop2(); void op_cop0();

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

    uint32_t current_pc, pc, next_pc;
    uint32_t registers[32];
    uint32_t hi, lo;

    bool is_branch, is_delay_slot;
    bool took_branch;
    bool in_delay_slot_took_branch;

    uint32_t exception_addr[2] = { 0x80000080, 0xBFC00180 };

    /* Coprocessors. */
    Cop0 cop0;

    MEM write_back, memory_load;
    MEM delayed_memory_load;

    Instr instr;

    //debug expansion and exe
    bool debug = false;
    bool isEX1 = true;
    bool exe = true;

    /* Opcode lookup table. */
    std::unordered_map<uint32_t, CPUfunc> lookup, special;
};