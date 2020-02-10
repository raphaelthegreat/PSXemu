#pragma once
#include <cstdint>

#define NUM_TIMERS 3

enum class ClockSource : uint32_t {
	SysClock,
	SysClockDiv8,
	GpuDotClock,
	GpuHSync,
};

enum class SyncMode : uint32_t
{
	PauseOnGate = 0,
	ResetOnGate = 1,
	ResetAndRunOnGate = 2,
	FreeRunOnGate = 3
};

union CounterValue {
	uint32_t raw;

	struct {
		uint32_t value : 16;
		uint32_t reserved : 16;
	};
};

union CounterControl {
	uint32_t raw;

	struct {
		uint32_t sync_enable : 1;
		SyncMode sync_mode : 2;
		uint32_t reset : 1;
		uint32_t irq_when_target : 1;
		uint32_t irq_when_overflow : 1;
		uint32_t irq_once_repeat : 1;
		uint32_t irq_pulse_toggle : 1;
		uint32_t clock_source : 2;
		uint32_t irq_request : 1;
		uint32_t reached_target : 1;
		uint32_t reached_overflow : 1;
		uint32_t reserved : 19;
	};
};

union CounterTarget {
	uint32_t raw;

	struct {
		uint32_t target : 16;
		uint32_t reserved : 16;
	};
};

struct CounterState {
	CounterTarget target;
	CounterValue counter;
	CounterControl mode;
	bool gate;
	bool use_external_clock;
	bool external_counting_enabled;
	bool counting_enabled;
};

class Timers {
public:
	Timers() = default;
	~Timers() = default;

	void reset();
	void step(uint32_t cycles);

	void write(uint32_t offset, uint32_t val);
	uint32_t read(uint32_t offset);

private:
	CounterState timers[NUM_TIMERS] = {};
};

struct TimeManager {
	uint64_t time;
};