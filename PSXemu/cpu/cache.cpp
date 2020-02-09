#include "cache.h"

uint32_t CacheLine::tag()
{
	return tag_val & 0xfffff000;
}

uint32_t CacheLine::tag_valid()
{
	return (tag_val >> 2) & 0x7;
}

void CacheLine::set_tag_valid(uint32_t pc)
{
	tag_val = pc & 0xfffff00c;
}

void CacheLine::invalidate()
{
	tag_val |= 0x10;
}
