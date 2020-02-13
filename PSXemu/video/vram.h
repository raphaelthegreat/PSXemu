#pragma once
#include <cstdint>

union Pixel {
	uint16_t raw;
	
	struct {
		uint16_t ll : 4;
		uint16_t ml : 4;
		uint16_t mr : 4;
		uint16_t rr : 4;
	};
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

struct VRAM {
public:
	VRAM() = default;

	uint16_t read(int x, int y);
	void write(int x, int y, uint16_t data);

public:
	Pixel buffer[1024][512];
};