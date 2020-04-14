#pragma once
#include <memory/range.h>

const Range RAM = Range(0x00000000, 2 * 1024 * 1024LL);

struct PSEXELoadInfo {
	uint pc;
	uint r28;
	uint r29_r30;
};

class Ram {
public:
	Ram() = default;
	~Ram() = default;

	PSEXELoadInfo executable();

	template <typename T>
	T read(uint address);

	template <typename T>
	void write(uint address, T value);

	ubyte buffer[2048 * 1024] = {};
};

template<typename T>
inline T Ram::read(uint address)
{
	T* data = (T*)buffer;
	int index = RAM.offset(address) / sizeof(T);
	return data[index];
}

template<typename T>
inline void Ram::write(uint address, T value)
{
	T* data = (T*)buffer;
	int index = RAM.offset(address) / sizeof(T);
	data[index] = value;
}
