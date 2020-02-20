#include "vram.h"

uint16_t VRAM::read(int x, int y)
{
	return buffer[x][y].raw;
}

void VRAM::write(int x, int y, uint16_t data)
{
	buffer[x][y].raw = data;
}
