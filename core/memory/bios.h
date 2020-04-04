#pragma once
#include "range.h"

const Range BIOS = Range(0x1fc00000, 512 * 1024);

class Bios {
public:
	Bios(std::string path);
	~Bios() = default;

	template <typename T>
	T read(uint offset);

public:
	ubyte bios[512 * 1024] = { 0x0 };
};

template<typename T>
inline T Bios::read(uint address)
{
	uint offset = address - BIOS.start;
	
	T val = 0;
    for (int i = 0; i < sizeof(T); i++) {
        val |= (bios[offset + i] << 8 * i);
    }

    return val;
}
