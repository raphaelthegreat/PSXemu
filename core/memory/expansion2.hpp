#pragma once
#include <utility/types.hpp>
#include <memory/bus.h>

class Expansion2 {
public:
	Expansion2(Bus* _bus) :
		bus(_bus) {}
	~Expansion2() = default;

	template <typename T>
	T read(uint address);

	template <typename T>
	void write(uint address, T data);

public:
	ubyte post = 0;
	Bus* bus;
};

template<typename T>
inline void Expansion2::write(uint address, T data)
{
	int offset = bus->EXPANSION_2.offset(address);
	/* Write to POST register. */
	if (offset == 0x41) {
		util::write_memory<T>((ubyte*)&post, offset - 0x41, data);
	}
}
