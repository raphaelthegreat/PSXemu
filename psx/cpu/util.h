#pragma once
#include <cstdint>
#include <bitset>

/* Returns a binary number from bit start up to but not including bit end. */
uint32_t bit_range(uint32_t num, int start, int end);
/* Set the specified bit range to <range> from bit start up to but not including bit end */
uint32_t set_bit_range(uint32_t num, int start, int end, uint32_t range);

bool get_bit(uint32_t num, int b);
uint32_t set_bit(uint32_t num, int b, bool v);

void panic(const char* msg, const char* val);
void panic(const char* msg, uint32_t hexval);

void log(const char* msg, uint32_t hexval);

/* Template functions. */
template <int bitcount = 16>
std::string binary(uint32_t num)
{
	std::bitset<bitcount> b(num);
	return "0b" + b.to_string();
}
