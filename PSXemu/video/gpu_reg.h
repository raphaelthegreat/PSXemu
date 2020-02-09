#pragma once
#include <cstdint>

enum class TextureDepth {
	D4bit = 0,
	D8bit = 1,
	D15bit = 2
};

enum class Field {
	Top = 1,
	Bottom = 0
};

class HorizontalRes {
public:
	static HorizontalRes from_bits(uint8_t h1, uint8_t h2);

public:
	uint8_t hr : 3;
};

enum class VerticalRes {
	V240 = 0,
	V480 = 1
};

enum class VideoMode {
	NTSC = 0,
	PAL = 1
};

enum class ColorDepth {
	C15bit = 0,
	C24bit = 1
};

enum class DMADirection {
	Off = 0,
	Fifo = 1,
	Cpu_Gpu = 2,
	Vram_Cpu = 3
};

struct GPUSTAT {
	uint32_t page_base_x;
	uint32_t page_base_y;
	uint32_t semi_transprency;

	TextureDepth texture_depth;
	uint32_t dithering;
	uint32_t draw_to_display;
	uint32_t force_set_mask_bit;
	uint32_t preserve_masked_pixels;

	Field field;
	uint32_t texture_disable;
	HorizontalRes hres;
	VerticalRes vres;
	VideoMode video_mode;

	ColorDepth color_depth;
	uint32_t vertical_interlace;
	uint32_t display_disable;
	uint32_t interrupt_req;
	DMADirection dma_dir;
};