#pragma once
#include <cstdint>

enum class ClockSource : uint32_t {
	SysClock,
	SysClockDiv8,
	GpuDotClock,
	GpuHSync,
};

enum class Sync : uint32_t {
	Pause = 0,
	Reset = 1,
	ResetAndPause = 2,
	WaitForSync = 3,
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
		Sync sync_mode : 2;
		uint32_t reset : 1;
		uint32_t irq_when_target : 1;
		uint32_t irq_when_overflow : 1;
		uint32_t irq_once_repeat : 1;
		uint32_t irq_pulse_toggle : 1;
		ClockSource clock_source : 2;
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

class Timer {
public:
	Timer() = default;
	~Timer() = default;

	void step(uint32_t cycles);

	void write(uint32_t offset, uint32_t val);
	uint32_t read(uint32_t offset);

private:
	CounterControl control;
	CounterValue value;
	CounterTarget target;

	bool paused = false;
	uint32_t count = 0;
	int32_t type;
};

struct TimeManager {
	uint64_t time;
};