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
    CONTR = 7,
    SIO = 8,
    SPU = 9,
    PIO = 10
};
