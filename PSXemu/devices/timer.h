#pragma once
#include <cstdint>

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
		uint32_t sync_mode : 2;
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