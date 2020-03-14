#include "vram.h"
#include <cstdlib>

uint16_t* VRAM::get()
{
	return &vram.halfword[0];
}

uint16_t VRAM::read(int x, int y)
{
	int index = (y * 1024) + x;
	return vram.halfword[index];
}

void VRAM::write(int x, int y, uint16_t data)
{
	if (x < 0 || x > 0x400) exit(0);
	if (y < 0 || y > 0x200) exit(0);

	vram.halfword[(y * 1024) + x] = data;
}

VRAM vram;