#pragma once
#include <cstdint>
#include <cpu/enum.h>

enum class ExceptionType {
    Interrupt = 0x0,
    ReadError = 0x4,
    WriteError = 0x5,
    BusError = 0x6,
    Syscall = 0x8,
    Break = 0x9,
    IllegalInstr = 0xA,
    CoprocessorError = 0xB,
    Overflow = 0xC
};

enum class Interrupt {
    VBLANK = 0,
    GPU_IRQ = 1,
    CDROM = 2,
    DMA = 3,
    TIMER0 = 4,
    TIMER1 = 5,
    TIMER2 = 6,
    CONTROLLER = 7,
    SIO = 8,
    SPU = 9,
    PIO = 10
};

struct EXE_HEADER {
    unsigned char id[8];
    uint32_t text;
    uint32_t data;
    uint32_t pc0;
    uint32_t gp0;
    uint32_t t_addr;
    uint32_t t_size;
    uint32_t d_addr;
    uint32_t d_size;
    uint32_t b_addr;
    uint32_t b_size;
    uint32_t s_addr;
    uint32_t s_size;
    uint32_t SavedSP;
    uint32_t SavedFP;
    uint32_t SavedGP;
    uint32_t SavedRA;
    uint32_t SavedS0;
};