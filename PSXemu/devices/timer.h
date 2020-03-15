#pragma once
#include <cstdint>
#include <cpu/enum.h>

#define NUM_TIMERS 3

enum class TimerID {
	TMR0 = 0,
	TMR1 = 1,
	TMR2 = 2
};

enum class SyncMode : uint32_t {
	/* For timers 0 and 1. */
	Pause = 0,
	Reset = 1,
	ResetPause = 2,
	PauseFreeRun = 3,

	/* For timer 2. */
	FreeRun = 4,
	Stop = 5
};

enum class ClockSrc : uint32_t {
	System,
	SystemDiv8,
	Dotclock,
	Hblank,
};

enum class ResetWhen : uint32_t {
	Overflow = 0,
	Target = 1
};

enum class IRQRepeat : uint32_t {
	OneShot = 0,
	Repeatedly = 1
};

enum class IRQToggle : uint32_t {
	Pulse = 0,
	Toggle = 1
};

union CounterValue {
	uint32_t raw;

	struct {
		uint32_t value : 16;
		uint32_t : 16;
	};
};

union CounterControl {
	uint32_t raw;

	struct {
		uint32_t sync_enable : 1;
		uint32_t sync_mode : 2;
		ResetWhen reset : 1;
		uint32_t irq_when_target : 1;
		uint32_t irq_when_overflow : 1;
		IRQRepeat irq_repeat_mode : 1;
		IRQToggle irq_pulse_mode : 1;
		uint32_t clock_source : 2;
		uint32_t irq_request : 1;
		uint32_t reached_target : 1;
		uint32_t reached_overflow : 1;
		uint32_t : 19;
	};
};

union CounterTarget {
	uint32_t raw;

	struct {
		uint32_t target : 16;
		uint32_t : 16;
	};
};

class TIMER {
public:
    int timerNumber;
    int timerCounter;

    uint32_t counterValue;
    uint32_t counterTargetValue;

    uint8_t syncEnable;
    uint8_t syncMode;
    uint8_t resetCounterOnTarget;
    uint8_t irqWhenCounterTarget;
    uint8_t irqWhenCounterFFFF;
    uint8_t irqRepeat;
    uint8_t irqPulse;
    uint8_t clockSource;
    uint8_t interruptRequest;
    uint8_t reachedTarget;
    uint8_t reachedFFFF;

    bool vblank;
    bool hblank;
    int dotDiv = 1;

    bool prevHblank;
    bool prevVblank;

    bool irq;
    bool alreadFiredIrq;

    TIMER() {
        this->timerNumber = timerCounter++;
    }

    template <typename T>
    void write(uint32_t addr, T value) {
        switch (addr & 0xF) {
        case 0x0: counterValue = (uint16_t)value; break;
        case 0x4: setCounterMode(value); break;
        case 0x8: counterTargetValue = value; break;
        //default: Console.WriteLine("[TIMER] " + timerNumber + "Unhandled Write" + addr); Console.ReadLine(); ; break;
        }
    }

    template <typename T>
    T read(uint32_t addr) {
        switch (addr & 0xF) {
        case 0x0: return counterValue;
        case 0x4: return getCounterMode();
        case 0x8: return counterTargetValue;
        //default: Console.WriteLine("[TIMER] " + timerNumber + "Unhandled load" + addr); Console.ReadLine(); return 0;
        }
    }

    //infact only timer 0 and 1 todo
    void syncGPU(std::tuple<int, bool, bool> sync) {
        
        auto [sdotDiv, shblank, svblank] = sync;
        
        prevHblank = hblank;
        prevVblank = vblank;
        dotDiv = sdotDiv;
        hblank = shblank;
        vblank = svblank;
    }

    int cycles;
    bool tick(int cyclesTicked) { //todo this needs rework
        cycles += cyclesTicked;
        switch (timerNumber) {
        case 0:
            if (syncEnable == 1) {
                switch (syncMode) {
                case 0: if (hblank) return false; break;
                case 1: if (hblank) counterValue = 0; break;
                case 2: if (hblank) counterValue = 0; if (!hblank) return false; break;
                case 3: if (!prevHblank && hblank) syncEnable = 0; else return false; break;
                }
            } //else free run

            if (clockSource == 0 || clockSource == 2) {
                counterValue += (uint16_t)cycles;
                cycles = 0;
            }
            else {
                uint16_t dot = (uint16_t)(cycles * 11 / 7 / dotDiv);
                counterValue += dot; //DotClock
                cycles = 0;
            }
            return handleIrq();

        case 1:
            if (syncEnable == 1) {
                switch (syncMode) {
                case 0: if (vblank) return false; break;
                case 1: if (vblank) counterValue = 0; break;
                case 2: if (vblank) counterValue = 0; if (!vblank) return false; break;
                case 3: if (!prevVblank && vblank) syncEnable = 0; else return false; break;
                }
            }

            if (clockSource == 0 || clockSource == 2) {
                counterValue += (uint16_t)cycles;
                cycles = 0;
            }
            else {
                counterValue += (uint16_t)(cycles / 2160);
                cycles %= 2160;
            }

            return handleIrq();
        case 2:
            if (syncEnable == 1 && syncMode == 0 || syncMode == 3) {
                return false; // counter stoped
            } //else free run

            if (clockSource == 0 || clockSource == 1) {
                counterValue += (uint16_t)cycles;
                cycles = 0;
            }
            else {
                counterValue += (uint16_t)(cycles / 8);
                cycles %= 8;
            }

            return handleIrq();
        default:
            return false;
        }

    }

    bool handleIrq() {
        irq = false;

        if (counterValue >= counterTargetValue) {
            reachedTarget = 1;
            if (resetCounterOnTarget == 1) counterValue = 0;
            if (irqWhenCounterTarget == 1) irq = true;
        }

        if (counterValue >= 0xFFFF) {
            reachedFFFF = 1;
            if (irqWhenCounterFFFF == 1) irq = true;
        }

        counterValue &= 0xFFFF;

        if (!irq) return false;

        if (irqPulse == 0) { //short bit10
            interruptRequest = 0;
        }
        else { //toggle it
            interruptRequest = (uint8_t)((interruptRequest + 1) & 0x1);
        }

        bool trigger = interruptRequest == 0;

        if (irqRepeat == 0) { //once
            if (!alreadFiredIrq && trigger) {
                alreadFiredIrq = true;
            }
            else { //already fired
                return false;
            }
        } // repeat

        interruptRequest = 1;

        return trigger;
    }

    void setCounterMode(uint32_t value) {
        syncEnable = (uint8_t)(value & 0x1);
        syncMode = (uint8_t)((value >> 1) & 0x3);
        resetCounterOnTarget = (uint8_t)((value >> 3) & 0x1);
        irqWhenCounterTarget = (uint8_t)((value >> 4) & 0x1);
        irqWhenCounterFFFF = (uint8_t)((value >> 5) & 0x1);
        irqRepeat = (uint8_t)((value >> 6) & 0x1);
        irqPulse = (uint8_t)((value >> 7) & 0x1);
        clockSource = (uint8_t)((value >> 8) & 0x3);

        interruptRequest = 1;
        alreadFiredIrq = false;

        counterValue = 0;
    }

    uint32_t getCounterMode() {
        uint32_t counterMode = 0;
        counterMode |= syncEnable;
        counterMode |= (uint32_t)syncMode << 1;
        counterMode |= (uint32_t)resetCounterOnTarget << 3;
        counterMode |= (uint32_t)irqWhenCounterTarget << 4;
        counterMode |= (uint32_t)irqWhenCounterFFFF << 5;
        counterMode |= (uint32_t)irqRepeat << 6;
        counterMode |= (uint32_t)irqPulse << 7;
        counterMode |= (uint32_t)clockSource << 8;
        counterMode |= (uint32_t)interruptRequest << 10;
        counterMode |= (uint32_t)reachedTarget << 11;
        counterMode |= (uint32_t)reachedFFFF << 12;

        reachedTarget = 0;
        reachedFFFF = 0;

        return counterMode;
    }
};

class Bus;
class TIMERS {
public:
	TIMER timer[3];

    template <typename T>
	void write(uint32_t addr, uint32_t value) {
		int timerNumber = (int)(addr & 0xF0) >> 4;
		timer[timerNumber].write<T>(addr, value);
		//Console.WriteLine("[TIMER] Write on" + ((addr & 0xF0) >> 4).ToString("x8") + " Value " + value.ToString("x8"));
	}

    template <typename T>
	uint32_t read(uint32_t addr) {
		int timerNumber = (int)(addr & 0xF0) >> 4;
		//Console.WriteLine("[TIMER] load on" + ((addr & 0xF0) >> 4).ToString("x8") + " Value " + timer[timerNumber].load(w, addr).ToString("x4"));
		return timer[timerNumber].read<T>(addr);
	}

	bool tick(int timerNumber, int cycles) {
		return timer[timerNumber].tick(cycles);
	}

	void syncGPU(std::tuple<int, bool, bool> sync) {
		timer[0].syncGPU(sync);
		timer[1].syncGPU(sync);
	}
};