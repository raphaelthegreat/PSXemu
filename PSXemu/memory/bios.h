#pragma once
#include <string>
#include <cstdint>

class Bios {
public:
	Bios(std::string path);
	~Bios() = default;

	template <typename T>
	T read(uint32_t offset);

public:
	uint8_t bios[512 * 1024] = { 0x0 };
};

template<typename T>
inline T Bios::read(uint32_t offset)
{
    T val = 0;
    for (int i = 0; i < sizeof(T); i++) {
        val |= (bios[offset + i] << 8 * i);
    }

    return val;
}
