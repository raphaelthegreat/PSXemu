#pragma once
#include "opengl/texture.h"
#include "gpu_reg.h"
#include <vector>
#include <functional>
#include <utility>
#include <cstdint>

#define BIND(x) std::bind(&GPU::x, this)

enum GP0Command {
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

enum GP1Command {
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

enum class TextureMethod {
	None,
	Raw,
	Blended,
};

typedef std::function<void()> GP0Func;

struct TCacheLine {
	uint8_t data[8];
};

struct VRAMLine {
public:
	VRAMLine() = default;
	~VRAMLine() = default;

	uint8_t get4bit(uint32_t index);
	uint8_t get8bit(uint32_t index);
	uint16_t get16bit(uint32_t index);
	uint32_t get24bit(uint32_t index);

private:
	uint8_t data[2048] = { 0 };
};

class Renderer;
class GPU {
public:
	GPU(Renderer* renderer);

	uint32_t get_status();
	uint32_t get_read();

	void gp0_command(uint32_t data);
	void gp1_command(uint32_t data);

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

	std::vector<uint32_t> command_fifo;
	GP0Func command_handler;
	GP0Func gp0_image_handler;
	uint32_t remaining_attribs;

	bool image_load;
	Renderer* gl_renderer;

	VRAMLine vram[512];
	TCacheLine texture_cache[256];

	/* Intermediate texture storage. */
	TextureBuffer buffer;
};