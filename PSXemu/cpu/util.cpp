#include "util.h"
#include <iostream>
#include <bitset>

uint32_t bit_range(uint32_t num, int start, int end)
{
	std::bitset<32> number(num);
	std::bitset<32> range(0);

	for (int i = 0; i < end - start; i++)
		range[i] = number[i + start];

	return range.to_ulong();
}

uint32_t set_bit_range(uint32_t num, int start, int end, uint32_t _range)
{
	std::bitset<32> number(num);
	std::bitset<32> range(_range);
	
	for (int i = 0; i < end - start; i++) 
		number[i + start] = range[i];

	return number.to_ulong();
}

bool get_bit(uint32_t num, int b)
{
	std::bitset<32> number(num);
	return number[b];
}

uint32_t set_bit(uint32_t num, int b, bool v)
{
	std::bitset<32> number(num);
	number[b] = v;

	return number.to_ulong();
}

void panic(const char* msg, const char* val)
{
	std::cerr << msg << val << '\n';
	exit(0);
}

void panic(const char* msg, uint32_t val)
{
	std::cerr << msg << std::hex << val << '\n';
	exit(0);
}

void log(const char* msg, uint32_t hexval)
{
	std::clog << msg << std::hex << hexval << '\n';
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 