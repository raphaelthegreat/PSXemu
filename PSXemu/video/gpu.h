#pragma once
#include "opengl/texture.h"
#include <video/rasterizer.h>
#include <video/gpu_reg.h>
#include <video/vram.h>

#include <glm/glm.hpp>
#include <functional>
#include <vector>

#define BIND(x) std::bind(&GPU::x, this)

constexpr uint32_t CPU_CYCLES_PER_SECOND = 33868800;
constexpr uint32_t FRAMERATE_NTSC = 60;
constexpr uint32_t CPU_CYCLES_PER_FRAME = CPU_CYCLES_PER_SECOND / FRAMERATE_NTSC;

class DataMover {
public:
	DataMover() = default;
	~DataMover() = default;

	void advance();

	uint32_t start_x, start_y;
	uint32_t x, y;

	uint32_t width, height;

	uint32_t pixel_count;
	bool active;
};

enum GP0Command {
	Nop = 0x0,
	Clear_Cache = 0x1,
	Fill_Rect = 0x2,
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

enum GPUMode {
	Command = 0,
	VRAMLoad = 1,
	VRAMStore = 2,
	VRAMCopy = 3
};

typedef std::function<void()> GP0Func;

/* GPU class. */
class Renderer;
class GPU {
public:
	GPU(Renderer* renderer);
	~GPU();

	uint32_t get_status();
	uint32_t get_read();

	/* Return clock timings based on PAL/NTSC. */
	uint16_t video_mode_timings();
	/* Return vertical lines per frame based on PAL/NTSC. */
	uint16_t lines_per_frame();

	/* Emulate GPU cycles. */
	bool tick(uint32_t cycles);

	/* Copy data from ram to vram. */
	void vram_load(uint16_t data);
	/* Copy data from vram to ram. */
	void vram_store(uint16_t data);
	/* Copy data from vram to vram. */
	void vram_copy(uint16_t data);

	/* Unpack color channels from GPU data. */
	glm::ivec3 unpack_color(uint32_t color);
	glm::ivec2 unpack_point(uint32_t point);
	
	/* Creates a pixel struct from data given. */
	/* NOTE: A texture coord is optional and only used in textured pixels. */
	Pixel create_pixel(uint32_t point, uint32_t color, uint32_t coord = 0);

	/* GPU write. */
	void gp0_command(uint32_t data);
	void gp1_command(uint32_t data);

	/* GP0 commands. */
	void gp0_nop();
	void gp0_mono_quad();
	void gp0_draw_mode();
	void gp0_fill_rect();
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

	/* GP1 commands. */
	void gp1_reset(uint32_t data);
	void gp1_display_mode(uint32_t data);
	void gp1_dma_dir(uint32_t data);
	void gp1_horizontal_display_range(uint32_t data);
	void gp1_vertical_display_range(uint32_t data);
	void gp1_start_of_display_area(uint32_t data);
	void gp1_display_enable(uint32_t data);
	void gp1_acknowledge_irq(uint32_t data);
	void gp1_reset_cmd_buffer(uint32_t data);
	void gp1_gpu_info(uint32_t data);

public:
	GPUSTAT status;
	GPURead read;

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
	uint32_t x_offset, y_offset;
	uint32_t gpu_clock, scanline, frames;

	std::vector<uint32_t> command_fifo;
	GP0Func command_handler;
	uint32_t remaining_attribs;

	bool in_vblank;
	GPUMode gpu_mode;
	Renderer* gl_renderer;

	/* Hold info about current move operation. */
	DataMover data_mover;

	/* GPU VRAM buffer. */
	VRAM vram;

	/* Used for rasterizing graphics into vram framebuffer. */
	Rasterizer raster;
};