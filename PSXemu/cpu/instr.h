#pragma once
#include <cstdint>

struct Instr {
    uint32_t value;                     //debug
    uint32_t opcode() { return value >> 26; }     //Instr opcode

    //I-Type
    uint32_t rs() { return (value >> 21) & 0x1F; }//Register Source
    uint32_t rt() { return(value >> 16) & 0x1F; }//Register Target
    uint32_t imm() { return value & 0xFFFF; } //Immediate value
    uint32_t imm_s() { return(uint32_t)(int16_t)imm(); } //Immediate value sign extended

    //R-Type
    uint32_t rd() { return(value >> 11) & 0x1F; }
    uint32_t sa() { return(value >> 6) & 0x1F; } //Shift Amount
    uint32_t function() { return value & 0x3F; }  //Function

    //J-Type                                       
    uint32_t addr() { return value & 0x3FFFFFF; } //Target Address

    //id / Cop
    uint32_t id() { return opcode() & 0x3; } //This is used mainly for coprocesor opcode id but its also used on opcodes that trigger exception

    void Decode(uint32_t instr) {
        value = instr;
    }
};