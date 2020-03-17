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
	vram.halfword[(y * 1024) + x] = data;
}

VRAM vram;