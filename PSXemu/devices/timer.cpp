#include "timer.h"
#include <cstdio>

void Timers::step(uint32_t cycles)
{
  
}

void Timers::reset()
{
	for (auto& timer : timers) {
		timer.counter.raw = 0;
		timer.counting_enabled = true;
		timer.external_counting_enabled = false;
		timer.gate = false;
		timer.mode.raw = 0;
		timer.target.raw = 0;
	}
}

void Timers::write(uint32_t offset, uint32_t val)
{
	uint32_t timer = (offset >> 4) & 3;
	uint32_t off = offset & 0xf;
	printf("Write to timer: %d at offset: %d with data: 0x%x\n", timer, off, val);

	CounterState& s = timers[timer];

	switch (off) {
	case 0: /* Write to Counter value. */
		s.counter.raw = val;
		break;
	case 4: /* Write to Counter control. */
		s.mode.raw = val;
		s.use_external_clock = (s.mode.clock_source & (timer == 2 ? 2 : 1)) != 0;
		s.counter.value = 0; /* Counter gets reset after write to Counter control. */
		break;
	case 8: /* Write to Counter target. */
		s.target.raw = val;
		break;
	}
}

uint32_t Timers::read(uint32_t offset)
{
	uint32_t timer = (offset >> 4) & 3;
	uint32_t off = offset & 0xf;
	printf("Read to timer: %d at offset: %d\n", timer, off);

	CounterState& s = timers[timer];

	switch (off) {
	case 0: /* Write to Counter value. */
		return s.counter.raw;
		break;
	case 4: /* Write to Counter control. */
		return s.mode.raw;
	case 8: /* Write to Counter target. */
		return s.target.raw;
	}
}
