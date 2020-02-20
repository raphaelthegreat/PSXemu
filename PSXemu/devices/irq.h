#pragma once
#include <cstdint>

enum class ExceptionType{
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
    VBLANK = 0x1,
    GPU_IRQ = 0x2,
    CDROM = 0x4,
    DMA = 0x8,
    TIMER0 = 0x10,
    TIMER1 = 0x20,
    TIMER2 = 0x40,
    CONTR = 0x80,
    SIO = 0x100,
    SPU = 0x200,
    PIO = 0x400
};

enum class Width {
    WORD,
    BYTE,
    HALF
};

class InterruptController {
public:
    uint32_t ISTAT; //IF Trigger that needs to be ack
    uint32_t IMASK; //IE Global Interrupt enable

    void set(Interrupt interrupt) {
        ISTAT |= (uint32_t)interrupt;
        //Console.WriteLine("ISTAT SET MANUAL FROM DEVICE: " + ISTAT.ToString("x8") + " IMASK " + IMASK.ToString("x8"));
    }


    void writeISTAT(uint32_t value) {
        ISTAT &= value & 0x7FF;
        //Console.ForegroundColor = ConsoleColor.Magenta;
        //Console.WriteLine("[IRQ] [ISTAT] Write " + value.ToString("x8") + " ISTAT " + ISTAT.ToString("x8"));
        //Console.ResetColor();
        //Console.ReadLine();
    }

    void writeIMASK(uint32_t value) {
        IMASK = value & 0x7FF;
        //Console.WriteLine("[IRQ] [IMASK] Write " + IMASK.ToString("x8"));
        //Console.ReadLine();
    }

    uint32_t loadISTAT() {
        //Console.WriteLine("[IRQ] [ISTAT] Load " + ISTAT.ToString("x8"));
        //Console.ReadLine();
        return ISTAT;
    }

    uint32_t loadIMASK() {
        //Console.WriteLine("[IRQ] [IMASK] Load " + IMASK.ToString("x8"));
        //Console.ReadLine();
        return IMASK;
    }

    bool interruptPending() {
        return (ISTAT & IMASK) != 0;
    }
};