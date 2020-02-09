#pragma once
#include <cstdint>

enum TexColors : uint32_t {
	D4bit = 0,
	D8bit = 1,
	D15bit = 2
};

enum Field : uint32_t {
	Top = 1,
	Bottom = 0
};

enum VerticalRes : uint32_t {
	V240 = 0,
	V480 = 1
};

enum VideoMode :uint32_t {
	NTSC = 0,
	PAL = 1
};

enum ColorDepth :uint32_t {
	C15bit = 0,
	C24bit = 1
};

enum DMADirection : uint32_t {
	Off = 0,
	Fifo = 1,
	Cpu_Gpu = 2,
	Vram_Cpu = 3
};

union GPUSTAT {
	uint32_t raw;

	struct {
		uint32_t page_base_x : 4;
		uint32_t page_base_y : 1;
		uint32_t semi_transprency : 2;
		uint32_t texture_depth : 2;
		uint32_t dithering : 1;
		uint32_t draw_to_display : 1;
		uint32_t force_set_mask_bit : 1;
		uint32_t preserve_masked_pixels : 1;
		uint32_t field : 1;
		uint32_t reverse_flag : 1;
		uint32_t texture_disable : 1;
		uint32_t hres : 3;
		uint32_t vres : 1;
		uint32_t video_mode : 1;
		uint32_t color_depth : 1;
		uint32_t vertical_interlace : 1;
		uint32_t display_disable : 1;
		uint32_t interrupt_req : 1;
		uint32_t dma_request : 1;
		uint32_t ready_cmd : 1;
		uint32_t ready_vram : 1;
		uint32_t ready_dma : 1;
		uint32_t dma_dir : 2;
		uint32_t even_odd_lines : 1;
	};
};