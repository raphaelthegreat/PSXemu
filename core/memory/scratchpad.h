#pragma once
#include <memory/range.h>

const Range SCRATCHPAD = Range(0x1f800000, 1024LL);

class Scratchpad {
public:
	Scratchpad() = default;
	~Scratchpad() = default;

	template <typename T>
	T read(uint address);

	template <typename T>
	void write(uint address, T value);

public:
	ubyte buffer[1024] = {};
};

template<typename T>
inline T Scratchpad::read(uint address)
{
	T* data = (T*)buffer;
	int index = SCRATCHPAD.offset(address) / sizeof(T);
	return data[index];
}

template<typename T>
inline void Scratchpad::write(uint address, T value)
{
	T* data = (T*)buffer;
	int index = SCRATCHPAD.offset(address) / sizeof(T);
	data[index] = value;
	return;
}