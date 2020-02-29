#include "vram.h"

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
	if (x < 0 || x > 0x400) return;
	if (y < 0 || y > 0x200) return;

	vram.halfword[(y * 1024) + x] = data;
}
