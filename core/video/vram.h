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

class VRAM {
public:
	VRAM() = default;
	~VRAM() = default;

	void init();
	void upload_to_gpu();

	void bind_vram_texture();

	ushort read(uint x, uint y);
	void write(uint x, uint y, ushort data);

public:
	uint pbo, texture;
	ushort* ptr;
};

extern VRAM vram;