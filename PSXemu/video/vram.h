#pragma once
#include <cstdint>

template <uint32_t size>
union Buffer {
	uint8_t byte[size];
	uint16_t halfword[size / 2];
	uint32_t word[size / 4];
};

union ClutAttrib {
	uint16_t raw;

	struct {
		uint16_t x : 6;
		uint16_t y : 9;
		uint16_t zr : 1;
	};
};

union TPageAttrib {
	uint16_t raw;

	struct {
		uint16_t page_x : 4;
		uint16_t page_y : 1;
		uint16_t semi_transp : 2;
		uint16_t page_colors : 2;
		uint16_t : 7;
	};
};

class VRAM {
public:
	VRAM() = default;
	~VRAM() = default;

	uint16_t read(uint32_t x, uint32_t y);
	void write(uint32_t x, uint32_t y, uint16_t data);

	uint16_t* get();

private:
	Buffer<1 << 20> vram;
};

extern VRAM vram;