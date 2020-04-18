#pragma once
#include <utility/types.hpp>

struct GPUSync {
	int dotDiv;
	bool hblank;
	bool vblank;
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

enum class TimerID {
	TMR0 = 0,
	TMR1 = 1,
	TMR2 = 2
};

enum class SyncMode : uint {
	/* For timers 0 and 1. */
	Pause = 0,
	Reset = 1,
	ResetPause = 2,
	PauseFreeRun = 3,

	/* For timer 2. */
	FreeRun = 4,
	Stop = 5
};

enum class ClockSrc : uint {
	System,
	SystemDiv8,
	Dotclock,
	Hblank,
};

enum class ResetWhen : uint {
	Overflow = 0,
	Target = 1
};

enum class IRQRepeat : uint {
	OneShot = 0,
	Repeatedly = 1
};

enum class IRQToggle : uint {
	Pulse = 0,
	Toggle = 1
};

union CounterValue {
	uint raw;

	struct {
		uint value : 16;
		uint : 16;
	};
};

union CounterControl {
	uint raw;

	struct {
		uint sync_enable : 1;
		uint sync_mode : 2;
		ResetWhen reset : 1;
		uint irq_when_target : 1;
		uint irq_when_overflow : 1;
		IRQRepeat irq_repeat_mode : 1;
		IRQToggle irq_pulse_mode : 1;
		uint clock_source : 2;
		uint irq_request : 1;
		uint reached_target : 1;
		uint reached_overflow : 1;
		uint : 19;
	};
};

union CounterTarget {
	uint raw;

	struct {
		uint target : 16;
		uint : 16;
	};
};

class Bus;
class Timer {
public:
	Timer(TimerID type, Bus* _bus);
	~Timer() = default;

	/* Trigger an interrupt. */
	void fire_irq();

	/* Add cycles to the timer. */
	void tick(uint cycles);
	void gpu_sync(GPUSync sync);

	/* Map timer to interrupt type. */
	Interrupt irq_type();

	/* Read/Write to the timer. */
	uint read(uint offset);
	void write(uint offset, uint data);

	/* Get the timer's clock source. */
	ClockSrc get_clock_source();

	/* Get the timer's sync mode. */
	SyncMode get_sync_mode();

public:
	CounterValue current;
	CounterControl mode;
	CounterTarget target;

	bool paused, already_fired_irq;
	bool in_hblank, in_vblank;
	bool prev_hblank, prev_vblank;
	uint count, dot_div = 0;

	TimerID timer_id;
	Bus* bus;
};