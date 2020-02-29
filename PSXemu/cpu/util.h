#pragma once
#include <cstdint>

/* Bit manipulation functions. */
uint32_t bit_range(uint32_t num, int start, int end);
uint32_t set_bit_range(uint32_t num, int start, int end, uint32_t range);

bool get_bit(uint32_t num, int b);
uint32_t set_bit(uint32_t num, int b, bool v);

/* Used to detect signed overflow in addition. */
bool safe_add(uint32_t* out, int a, int b);

/* Min/Max variadic template functions. */
template <typename T>
T min(T l) { return l; }

template <typename T>
T max(T l) { return l; }

template <typename T, typename... _Ty>
T min(T l, _Ty... other)
{
	T value = min(other...);
	return (l < value ? l : value);
}

template <typename T, typename... _Ty>
T max(T l, _Ty... other)
{
	T value = max(other...);
	return (l > value ? l : value);
}
