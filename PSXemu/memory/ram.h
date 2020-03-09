#pragma once
#include <cstdint>
#include <memory/range.h>

const Range RAM = Range(0x00000000, 2048 * 1024);

class Ram {
public:
	Ram() = default;
	~Ram() = default;
	
	template <typename T>
	T read(uint32_t address);

	template <typename T>
	void write(uint32_t address, T data);

	uint8_t data[2048 * 1024] = { 0x0 };
};

template<typename T>
inline T Ram::read(uint32_t address)
{
	T val = 0;
	for (int i = 0; i < sizeof(T); i++) {
		val |= (data[address + i] << 8 * i);
	}

	return val;
}

template<typename T>
inline void Ram::write(uint32_t address, T wdata)
{
	for (int i = 0; i < sizeof(T); i++) {
		data[address + i] = (T)(wdata >> 8 * i);
	}
}
