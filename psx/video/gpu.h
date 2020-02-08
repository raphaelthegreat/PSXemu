#pragma once
#include <vector>
#include <functional>
#include <utility>
#include <cstdint>

#define BIND(x) std::bind(&GPU::x, this)

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

enum class GP0Command {
	Nop = 0x0,
	Clear_Cache = 0x1,
	Mono_Quad = 0x28,
	Shaded_Quad_Blend = 0x2c,
	Shaded_Triangle = 0x30,
	Shaded_Quad = 0x38,
	Image_Load = 0xa0,
	Image_Store = 0xc0,
	Texture_Window_Setting = 0xe2,
	Draw_Mode_Setting = 0xe1,
	Draw_Area_Top_Left = 0xe3,
	Draw_Area_Bottom_Right = 0xe4,
	Drawing_Offset = 0xe5,
	Mask_Bit_Setting = 0xe6
};

enum class GP1Command {
	Reset = 0x0,
	Reset_Cmd_Buffer = 0x01,
	Acknowledge_Irq = 0x02,
	Display_Enable = 0x03,
	DMA_Direction = 0x04,
	Start_Of_Display_Area = 0x05,
	Horizontal_Display_Range = 0x06,
	Vertical_Display_Range = 0x07,
	Display_Mode = 0x08
};

typedef std::function<void()> GP0Func;

class Renderer;
class GPU {
public:
	GPU(Renderer* renderer);

	uint32_t get_status();
	uint32_t get_read();

	void write_gp0(uint32_t data);
	void write_gp1(uint32_t data);

	void gp0_nop();
	void gp0_mono_quad();
	void gp0_draw_mode();
	void gp0_draw_area_top_left();
	void gp0_draw_area_bottom_right();
	void gp0_texture_window_setting();
	void gp0_drawing_offset();
	void gp0_mask_bit_setting();
	void gp0_clear_cache();
	void gp0_image_load();
	void gp0_image_store();
	void gp0_shaded_quad();
	void gp0_shaded_quad_blend();
	void gp0_shaded_trig();

	void gp1_reset(uint32_t data);
	void gp1_display_mode(uint32_t data);
	void gp1_dma_dir(uint32_t data);
	void gp1_horizontal_display_range(uint32_t data);
	void gp1_vertical_display_range(uint32_t data);
	void gp1_start_of_display_area(uint32_t data);
	void gp1_display_enable(uint32_t data);
	void gp1_acknowledge_irq(uint32_t data);
	void gp1_reset_cmd_buffer(uint32_t data);

private:
	GPUSTAT status;
	
	bool texture_x_flip, texture_y_flip;
	uint8_t texture_window_x_mask;
	uint8_t texture_window_y_mask;

	uint8_t texture_window_x_offset;
	uint8_t texture_window_y_offset;

	uint16_t drawing_area_left;
	uint16_t drawing_area_top;
	uint16_t drawing_area_right;
	uint16_t drawing_area_bottom;

	int16_t drawing_x_offset;
	int16_t drawing_y_offset;

	uint16_t display_vram_x_start;
	uint16_t display_vram_y_start;

	uint16_t display_horiz_start;
	uint16_t display_horiz_end;
	uint16_t display_line_start;
	uint16_t display_line_end;

	std::vector<uint32_t> gp0_command;
	GP0Func gp0_handler;
	uint32_t gp0_remaining_words;

	bool image_load;
	Renderer* gl_renderer;

	uint8_t frame_buffer[1024 * 1024] = { 0x0 };
	uint8_t texture_cache[2 * 1024] = { 0x0 };
	uint8_t command_fifo[64];
};