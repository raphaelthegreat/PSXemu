#include "timer.h"
#include <cstdio>

void Timer::step(uint32_t cycles)
{
  
}

void Timer::write(uint32_t offset, uint32_t val)
{
	uint32_t off = offset & 0xf;
	printf("Timer write to offset: 0x%x with data: 0x%x\n", off, val);
	
	if (off == 0)
		value.raw = val;
	else if (off == 4) {
		control.raw = val;
		/* Counter value gets reset on counter mode write. */
		value.value = 0;
	}
	else if (off == 8)
		target.raw = val;
}

uint32_t Timer::read(uint32_t offset)
{
	uint32_t off = offset & 0xf;
	printf("Timer read to offset: 0x%x\n", off);
	
	if (off == 0)
		return value.raw;
	else if (off == 4)
		return control.raw;
	else if (off == 8)
		return target.raw;
}
