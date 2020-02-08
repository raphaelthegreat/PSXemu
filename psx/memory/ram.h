#pragma once
#include <cstdint>

class Ram {
public:
	Ram() = default;
	~Ram() = default;
	
	template <typename T>
	T read(uint32_t offset);

	template <typename T>
	void write(uint32_t offset, T data);

	uint8_t data[2048 * 1024] = { 0x0 };
};

template<typename T>
inline T Ram::read(uint32_t offset)
{
	T val = 0;
	for (int i = 0; i < sizeof(T); i++) {
		val |= (data[offset + i] << 8 * i);
	}

	return val;
}

template<typename T>
inline void Ram::write(uint32_t offset, T wdata)
{
	for (int i = 0; i < sizeof(T); i++) {
		data[offset + i] = (T)(wdata >> 8 * i);
	}
}
