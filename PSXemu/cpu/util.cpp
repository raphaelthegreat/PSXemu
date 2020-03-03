#include "util.h"
#include <iostream>
#include <bitset>

uint32_t bit_range(uint32_t num, int start, int end)
{
	uint32_t mask = (1 << end - start) - 1;
	num &= mask << start;
	return num >> start;
}

uint32_t set_bit_range(uint32_t num, int start, int end, uint32_t _range)
{
	uint32_t mask = (1 << end - start) - 1;

	num &= ~(mask << start);
	num |= ((_range & mask) << start);

	return num;
}

bool get_bit(uint32_t num, int b)
{
	return num & (1 << b);
}

uint32_t set_bit(uint32_t num, int b, bool v)
{
	if (v) num |= (1 << b);
	else num &= ~(1 << b);

	return num;
}

void panic(const char* msg, const char* val)
{
	std::cerr << msg << val << '\n';
	__debugbreak();
}

void panic(const char* msg, uint32_t val)
{
	std::cerr << msg << std::hex << val << '\n';
	__debugbreak();
}

void log(const char* msg, uint32_t hexval)
{
	std::clog << msg << std::hex << hexval << '\n';
}

bool safe_add(uint32_t* out, int a, int b)
{
	if (a >= 0) {
		if (b > (INT_MAX - a)) {
			/* Overflow */
			return false;
		}
	}
	else {
		if (b < (INT_MIN - a)) {
			/* Underflow */
			return false;
		}
	}

	*out = a + b;
	return true;
}
