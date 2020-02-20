#pragma once
#include <cstdint>

/* Returns a binary number from bit start up to but not including bit end. */
uint32_t bit_range(uint32_t num, int start, int end);
/* Set the specified bit range to <range> from bit start up to but not including bit end */
uint32_t set_bit_range(uint32_t num, int start, int end, uint32_t range);

bool get_bit(uint32_t num, int b);
uint32_t set_bit(uint32_t num, int b, bool v);

void panic(const char* msg, const char* val);
void panic(const char* msg, uint32_t hexval);

void log(const char* msg, uint32_t hexval);

bool safe_add(uint32_t* out, int a, int b);