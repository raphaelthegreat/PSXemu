#include "vram.h"
#include <cstdio>
#include <cstdlib>

uint16_t* VRAM::get()
{
	return &vram.halfword[0];
}

uint16_t VRAM::read(uint32_t x, uint32_t y)
{
	int index = (y * 1024) + x;
	return vram.halfword[index];
}

void VRAM::write(uint32_t x, uint32_t y, uint16_t data)
{
	if (x < 0 || x > 0x400) {
		printf("VRAM out of bound x write: %d\n", x); return;
	}
	if (y < 0 || y > 0x200) {
		printf("VRAM out of bound y write %d\n", y); return;
	}

	vram.halfword[(y * 1024) + x] = data;
}

VRAM vram;