#pragma once

union ClutAttrib {
	ushort raw;

	struct {
		ushort x : 6;
		ushort y : 9;
		ushort zr : 1;
	};
};

union TPageAttrib {
	ushort raw;

	struct {
		ushort page_x : 4;
		ushort page_y : 1;
		ushort semi_transp : 2;
		ushort page_colors : 2;
		ushort : 7;
	};
};

constexpr int VRAM_WIDTH = 1024;
constexpr int VRAM_HEIGHT = 512;

class VRAM {
public:
	VRAM() = default;
	~VRAM() = default;

	void init();
	void upload_to_gpu();

	void bind_vram_texture();
	void write_to_image();

	ushort read(uint x, uint y);
	void write(uint x, uint y, ushort data);

public:
	uint pbo, texture;
	ushort* ptr;

	ubyte* image_buffer;
};

extern VRAM vram;