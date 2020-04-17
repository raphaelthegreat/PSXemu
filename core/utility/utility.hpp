#pragma once
#include <string>
#include <utility/types.hpp>

namespace util {
	static uint bit_range(uint num, int start, int end)
	{
		uint mask = (1 << (end - start)) - 1;
		num &= mask << start;
		return num >> start;
	}

	static inline bool get_bit(uint num, int b)
	{
		return num & (1 << b);
	}

	static uint set_bit(uint num, int b, bool v)
	{
		if (v) num |= (1 << b);
		else num &= ~(1 << b);

		return num;
	}

	static inline bool sub_overflow(uint old_value, uint sub_value, uint new_value)
	{
		return (((new_value ^ old_value) & (old_value ^ sub_value)) & UINT32_C(0x80000000)) != 0;
	}

	static inline bool add_overflow(uint old_value, uint add_value, uint new_value)
	{
		return (((new_value ^ old_value) & (new_value ^ add_value)) & UINT32_C(0x80000000)) != 0;
	}

	static inline ubyte bcd_to_dec(ubyte val) 
	{
		return ((val / 16 * 10) + (val % 16));
	}

	static inline ubyte dec_to_bcd(ubyte val) 
	{
		return ((val / 10 * 16) + (val % 10));
	}

	template <typename T>
	static inline T min(T l)
	{ 
		return l; 
	}

	template <typename T, typename... _Ty>
	static inline T min(T l, _Ty... other)
	{
		T value = min(other...);
		return (l < value ? l : value);
	}

	template <typename T>
	static inline T max(T l)
	{ 
		return l; 
	}

	template <typename T, typename... _Ty>
	static inline T max(T l, _Ty... other)
	{
		T value = max(other...);
		return (l > value ? l : value);
	}

	template <typename _In, typename _Lower, typename _Upper>
	static inline _In clamp(_In val, _Lower lower, _Upper upper)
	{
		return val + ((val < lower) * (lower - val)) + ((val > upper) * (upper - val));
	}

	template<int bits>
	static inline uint sclip(uint value) {
		enum { mask = (1 << bits) - 1 };
		enum { sign = 1 << (bits - 1) };

		return ((value & mask) ^ sign) - sign;
	}

	template<int bits>
	static inline uint uclip(uint value) {
		enum { mask = (1 << bits) - 1 };
		enum { sign = 0 };

		return ((value & mask) ^ sign) - sign;
	}

	template <typename T>
	static inline T read_memory(void* memory, int offset)
	{
		T* data = (T*)memory;
		int index = offset / sizeof(T);
		return data[index];
	}

	template <typename T>
	static inline void write_memory(void* memory, int offset, T value)
	{
		T* data = (T*)memory;
		int index = offset / sizeof(T);
		data[index] = value;
	}

	static inline void read_binary_file(const std::string& file, ulong size, ubyte* target)
	{
		FILE* in = fopen(file.c_str(), "rb");
		fread(target, 1, size, in);
		fclose(in);
	}

}