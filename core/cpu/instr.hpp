#pragma once
#include <memory/range.h>

struct Instr {
    uint value;                     //debug
    uint opcode() { return value >> 26; }     //Instr opcode

    //I-Type
    uint rs() { return (value >> 21) & 0x1F; }//Register Source
    uint rt() { return(value >> 16) & 0x1F; }//Register Target
    uint imm() { return value & 0xFFFF; } //Immediate value
    uint imm_s() { return(uint)(int16_t)imm(); } //Immediate value sign extended

    //R-Type
    uint rd() { return(value >> 11) & 0x1F; }
    uint sa() { return(value >> 6) & 0x1F; } //Shift Amount
    uint function() { return value & 0x3F; }  //Function

    //J-Type                                       
    uint addr() { return value & 0x3FFFFFF; } //Target Address

    //id / Cop
    uint id() { return opcode() & 0x3; } //This is used mainly for coprocesor opcode id but its also used on opcodes that trigger exception
};