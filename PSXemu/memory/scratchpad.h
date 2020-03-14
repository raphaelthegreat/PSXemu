#pragma once
#include <cstdint>
#include "range.h"

const Range SCRATCHPAD = Range(0x1f800000, 1024);

class Scratchpad {
public:
	Scratchpad() = default;
	~Scratchpad() = default;

	template <typename T>
	T read(uint32_t address);

	template <typename T>
	void write(uint32_t address, T wdata);

public:
	uint8_t buffer[1024] = {};
};

template<typename T>
inline T Scratchpad::read(uint32_t address)
{
	uint32_t offset = SCRATCHPAD.offset(address);
	
	T val = 0;
	for (int i = 0; i < sizeof(T); i++) {
		val |= (buffer[offset + i] << 8 * i);
	}

	return val;
}

template<typename T>
inline void Scratchpad::write(uint32_t address, T wdata)
{
	uint32_t offset = SCRATCHPAD.offset(address);
	for (int i = 0; i < sizeof(T); i++) {
		buffer[offset + i] = (T)(wdata >> 8 * i);
	}
}