#include "gpu.h"
#include <cpu/util.h>
#include <cpu/cpu.h>
#include <algorithm>
#include <video/math_util.h>
#include <video/renderer.h>

void DataMover::advance()
{
	x++;
	if (x == width) {
		x = 0; y++;
	}
}

/* GPU class implementation */
GPU::GPU(Renderer* renderer) : raster(this)
{
	gl_renderer = renderer;
	
	status.raw = 0;
	gpu_mode = GPUMode::Command;
	drawing_area_left = 0;
	drawing_area_top = 0;
	drawing_area_right = 0;
	drawing_area_bottom = 0;
	display_vram_x_start = 0;
	display_vram_y_start = 0; 
	display_horiz_start = 0x200;
	display_horiz_end = 0xc00;
	display_line_start = 0x10;
	display_line_end = 0x100;
	remaining_attribs = 0;
	command_handler = BIND(gp0_nop);

	texture_window_x_mask = 0;
	texture_window_y_mask = 0;
	texture_window_x_offset = 0;
	texture_window_y_offset = 0;
	texture_x_flip = false;
	texture_y_flip = false;
	in_vblank = false;

	frames = 0;
}

GPU::~GPU()
{

}

uint32_t GPU::get_status()
{
	GPUSTAT copy = status;
	/* Hacks. */
	copy.reverse_flag = 0;
	copy.vres = VerticalRes::V240;
	
	/* Simulate that GPU is ready. */
	copy.ready_cmd = 1;
	copy.ready_dma = 1;
	copy.ready_vram = 1;

	switch (status.dma_dir) {
	case Off: 
		copy.dma_request = 0; 
		break;
	case Fifo: 
		copy.dma_request = 1; 
		break;
	case Cpu_Gpu: 
		copy.dma_request = copy.ready_dma; 
		break;
	case Vram_Cpu: 
		copy.dma_request = copy.ready_vram; 
		break;
	};

	return copy.raw;
}

uint32_t GPU::get_read()
{
	//printf("GPURead!\n");
	if (data_mover.active) {
		auto data = vram.read(data_mover.start_x + data_mover.x,
			data_mover.start_y + data_mover.y);

		data_mover.x++;

		if (data_mover.x == data_mover.width) {
			data_mover.x = 0;
			data_mover.y++;

			if (data_mover.y == data_mover.height) {
				data_mover.y = 0;
				data_mover.active = false;
			}
		}

		return data;
	}

	return 0;
}

uint16_t GPU::video_mode_timings()
{
	if (status.video_mode == VideoMode::NTSC)
		return 3412;
	else
		return 3404;
}

uint16_t GPU::lines_per_frame()
{
	if (status.video_mode == VideoMode::NTSC)
		return 263;
	else
		return 314;
}

bool GPU::tick(uint32_t cycles)
{
	gpu_clock += cycles;

	int newLines = gpu_clock / 3413;
	if (newLines == 0) return false;
	gpu_clock %= 3413;
	scanline += newLines;

	if (scanline < 242) {
		if (status.vres == VerticalRes::V480 && status.vertical_interlace) {
			status.odd_lines = (frames % 2) != 0;
		}
		else {
			status.odd_lines = (scanline % 2) != 0;
		}
	}
	else {
		status.odd_lines = false;
	}
	
	if (scanline == 262) {
		scanline = 0;
		frames++;
		
		return true;
	}
	
	return false;
}

void GPU::vram_load(uint16_t data)
{
	if (data_mover.active == true) {
		/* Write first halfword to vram. */
		uint32_t vramx = data_mover.start_x + data_mover.x;
		uint32_t vramy = data_mover.start_y + data_mover.y;

		/* Write to vram. */
		vram.write(vramx, vramy, data);

		/* Increment 2D coords. */
		data_mover.x++;
		if (data_mover.x == data_mover.width) {
			/* Move to next vram line. */
			data_mover.x = 0;
			data_mover.y++;

			/* Transfer complete. */
			if (data_mover.y == data_mover.height && remaining_attribs == 0) {
				data_mover.y = 0;
				data_mover.active = false;
				gpu_mode = Command;
			}
		}
	}
}

void GPU::vram_store(uint16_t data)
{
}

void GPU::vram_copy(uint16_t data)
{
}

glm::ivec3 GPU::unpack_color(uint32_t color)
{
	/* Unpack bits. */
	glm::ivec3 result;
	result.r = (color >> 0) & 0xff;
	result.g = (color >> 8) & 0xff;
	result.b = (color >> 16) & 0xff;

	return result;
}

glm::ivec2 GPU::unpack_point(uint32_t point)
{
	glm::ivec2 point2;
	point2.x = bit_range(point, 0, 11);
	point2.y = bit_range(point, 16, 27);

	return point2;
}

Pixel GPU::create_pixel(uint32_t point, uint32_t color, uint32_t coord)
{
	Pixel pixel;
	pixel.point = unpack_point(point);
	pixel.color = unpack_color(color);

	pixel.u = (coord >> 0) & 0xff;
	pixel.v = (coord >> 8) & 0xff;

	return pixel;
}

void GPU::gp0_command(uint32_t data)
{
	/* Handle a new GPU command. */
	if (remaining_attribs == 0) {
		GP0Command command = (GP0Command)bit_range(data, 24, 32);

		/* Number of operands and callback function of command. */
		std::pair<uint32_t, GP0Func> handler;
		
		/* Select the appropriate handler. */
		switch (command) {
		case Nop:
			handler = std::make_pair(1, BIND(gp0_nop));
			break;
		case Fill_Rect:
			handler = std::make_pair(3, BIND(gp0_fill_rect));
			break;
		case Shaded_Quad:
			handler = std::make_pair(8, BIND(gp0_shaded_quad));
			break;
		case Shaded_Quad_Blend:
			handler = std::make_pair(9, BIND(gp0_shaded_quad_blend));
			break;
		case Shaded_Triangle:
			handler = std::make_pair(6, BIND(gp0_shaded_trig));
			break;
		case Image_Load:
			handler = std::make_pair(3, BIND(gp0_image_load));
			break;
		case Image_Store:
			handler = std::make_pair(3, BIND(gp0_image_store));
			break;
		case Clear_Cache:
			handler = std::make_pair(1, BIND(gp0_clear_cache));
			break;
		case Mono_Quad:
			handler = std::make_pair(5, BIND(gp0_mono_quad));
			break;
		case Draw_Area_Bottom_Right:
			handler = std::make_pair(1, BIND(gp0_draw_area_bottom_right));
			break;
		case Draw_Area_Top_Left:
			handler = std::make_pair(1, BIND(gp0_draw_area_top_left));
			break;
		case Drawing_Offset:
			handler = std::make_pair(1, BIND(gp0_drawing_offset));
			break;
		case Draw_Mode_Setting:
			handler = std::make_pair(1, BIND(gp0_draw_mode));
			break;
		case Texture_Window_Setting:
			handler = std::make_pair(1, BIND(gp0_texture_window_setting));
			break;
		case Mask_Bit_Setting:
			handler = std::make_pair(1, BIND(gp0_mask_bit_setting));
			break;
		default:
			//printf("Unhandled GP0 command: 0x%x\n", (uint32_t)command);
			exit(0);
		}

		/* Set command info. */
		remaining_attribs = handler.first;
		command_handler = handler.second;

		/* Prepare the fifo for a new set of data. */
		command_fifo.clear();
	}

	/* If we are here it means that we received command attribs. */
	remaining_attribs--;

	/* Push attrib to command fifo (first attrib is always the command). */
	if (gpu_mode == Command) {
		command_fifo.push_back(data);

		/* Once we gether everything we need, execute the command. */
		if (remaining_attribs == 0)
			command_handler();
	}
	else {
		/* Unpack batched pixels. */
		uint16_t pixel1 = bit_range(data, 0, 16);
		uint16_t pixel2 = bit_range(data, 16, 32);

		uint32_t vramx = data_mover.start_x + data_mover.x;
		uint32_t vramy = data_mover.start_y + data_mover.y;

		/* Select vram operation to do. */
		if (gpu_mode == VRAMLoad) {
			vram_load(pixel1);
			vram_load(pixel2);
		}
		else if (gpu_mode == VRAMStore)
			vram_store(data);
		else if (gpu_mode == VRAMCopy)
			vram_copy(data);

		/* Data transfer complete. */
		if (remaining_attribs == 0) {
			gpu_mode = Command;
		}
	}
}

void GPU::gp1_command(uint32_t data)
{
	GP1Command command = (GP1Command)bit_range(data, 24, 32);

	switch (command) {
	case Reset:
		return gp1_reset(data);
	case Acknowledge_Irq:
		return gp1_acknowledge_irq(data);
	case Display_Enable:
		return gp1_display_enable(data);
	case Reset_Cmd_Buffer:
		return gp1_reset_cmd_buffer(data);
	case DMA_Direction:
		return gp1_dma_dir(data);
	case Start_Of_Display_Area:
		return gp1_start_of_display_area(data);
	case Horizontal_Display_Range:
		return gp1_horizontal_display_range(data);
	case Vertical_Display_Range:
		return gp1_vertical_display_range(data);
	case Display_Mode:
		return gp1_display_mode(data);
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
	case 0x1E:
	case 0x1F:
		gp1_gpu_info(data);
	default:
		//printf("Unhandled GP1 command: 0x%x\n", (uint32_t)command);
		exit(0);
	}
}

/* execute a NOP command. */
void GPU::gp0_nop()
{
	//printf("GPU Nop\n");
	return;
}

void GPU::gp0_mono_quad()
{
	//printf("Draw Mono Quad\n");

	auto color = command_fifo[0];
	auto point1 = command_fifo[1];
	auto point2 = command_fifo[2];
	auto point3 = command_fifo[3];
	auto point4 = command_fifo[4];

	auto v0 = create_pixel(point1, color);
	auto v1 = create_pixel(point2, color);
	auto v2 = create_pixel(point3, color);
	auto v3 = create_pixel(point4, color);

	Quad quad = { v0, v1, v2, v3 };
	raster.draw_polygon_shaded(quad);
}

void GPU::gp0_draw_mode()
{
	//printf("GPU set draw mode\n");
	uint32_t data = command_fifo[0];

	status.page_base_x = bit_range(data, 0, 4);
	status.page_base_y = get_bit(data, 4);
	status.semi_transprency = bit_range(data, 5, 7);
	status.texture_depth = (TexColors)bit_range(data, 7, 9);
	status.dithering = get_bit(data, 9);
	status.draw_to_display = get_bit(data, 10);
	status.texture_disable = get_bit(data, 11);

	texture_x_flip = get_bit(data, 12);
	texture_y_flip = get_bit(data, 13);
}

void GPU::gp0_fill_rect()
{
	//printf("GPU Fill rect!\n");

	auto color = unpack_color(command_fifo[0]);
	auto point1 = unpack_point(command_fifo[1]);
	auto point2 = unpack_point(command_fifo[2]);
	
	//printf("Color: %d %d %d\n", color.x, color.y, color.z);
	//printf("Point1: %d %d\n", point1.x, point1.y);
	//printf("Point2: %d %d\n", point2.x, point2.y);

	point1.x = (point1.x + 0x0) & ~0xf;
	point2.x = (point2.x + 0xf) & ~0xf;

	for (int y = 0; y < point2.y; y++) {
		for (int x = 0; x < point2.x; x++) {
			raster.draw_point(point1.x + x,
				point1.y + y,
				color.r,
				color.g,
				color.b);
		}
	}
}

void GPU::gp0_draw_area_top_left()
{
	//printf("GPU draw area top left\n");
	uint32_t data = command_fifo[0];

	drawing_area_top = (uint16_t)bit_range(data, 10, 20);
	drawing_area_left = (uint16_t)bit_range(data, 0, 10);
}

void GPU::gp0_draw_area_bottom_right()
{
	//printf("GPU draw area bottom right\n");
	uint32_t data = command_fifo[0];

	drawing_area_bottom = (uint16_t)bit_range(data, 10, 20);
	drawing_area_right = (uint16_t)bit_range(data, 0, 10);
}

void GPU::gp0_texture_window_setting()
{
	//printf("GPU texture window setting\n");
	uint32_t data = command_fifo[0];

	texture_window_x_mask = bit_range(data, 0, 5);
	texture_window_y_mask = bit_range(data, 5, 10);
	
	texture_window_x_offset = bit_range(data, 10, 15);
	texture_window_y_offset = bit_range(data, 15, 20);
}

void GPU::gp0_drawing_offset()
{
	//printf("GPU drawing offset\n");
	uint32_t data = command_fifo[0];
	
	uint16_t x = (uint16_t)(data & 0x7ff);
	uint16_t y = (uint16_t)((data >> 11) & 0x7ff);

	drawing_x_offset = ((int16_t)(x << 5)) >> 5;
	drawing_y_offset = ((int16_t)(y << 5)) >> 5;
	
	gl_renderer->set_draw_offset(drawing_x_offset, drawing_y_offset);
}

void GPU::gp0_mask_bit_setting()
{
	//printf("GPU mask bit setting\n");
	uint32_t data = command_fifo[0];

	status.force_set_mask_bit = get_bit(data, 0);
	status.preserve_masked_pixels = get_bit(data, 1);
}

void GPU::gp0_clear_cache()
{
	//printf("GPU clear cache\n");
}

void GPU::gp0_image_load()
{
	data_mover.start_x = command_fifo[1] & 0xffff;
	data_mover.start_y = command_fifo[1] >> 16;
	data_mover.width = command_fifo[2] & 0xffff;
	data_mover.height = command_fifo[2] >> 16;

	data_mover.x = 0;
	data_mover.y = 0;
	data_mover.active = true;

	remaining_attribs = data_mover.width * data_mover.height / 2;

	gpu_mode = VRAMLoad;
}

void GPU::gp0_image_store()
{
	uint32_t res = command_fifo[2];
	uint32_t width = bit_range(res, 0, 16);
	uint32_t height = bit_range(res, 16, 32);

	//printf("Unhandled image store: %d %d\n", width, height);
}

void GPU::gp0_shaded_quad()
{
	//printf("Draw Shaded Quad!\n");
	
	auto color1 = command_fifo[0];
	auto point1 = command_fifo[1];
	auto color2 = command_fifo[2];
	auto point2 = command_fifo[3];
	auto color3 = command_fifo[4];
	auto point3 = command_fifo[5];
	auto color4 = command_fifo[6];
	auto point4 = command_fifo[7];

	auto v0 = create_pixel(point1, color1);
	auto v1 = create_pixel(point2, color2);
	auto v2 = create_pixel(point3, color3);
	auto v3 = create_pixel(point4, color4);

	Quad quad = { v0, v1, v2, v3 };

	raster.draw_polygon_shaded(quad);
}

/* 1st  Color+Command     (CcBbGgRrh) (color is ignored for raw-textures)
   2nd  Vertex1           (YyyyXxxxh)
   3rd  Texcoord1+Palette (ClutYyXxh)
   4th  Vertex2           (YyyyXxxxh)
   5th  Texcoord2+Texpage (PageYyXxh)
   6th  Vertex3           (YyyyXxxxh)
   7th  Texcoord3         (0000YyXxh)
  (8th) Vertex4           (YyyyXxxxh) (if any)
  (9th) Texcoord4         (0000YyXxh) (if any)*/
void GPU::gp0_shaded_quad_blend()
{
	//printf("Draw Shaded Quad with Blending!\n");
	Quad quad;

	auto color = command_fifo[0];
	auto point1 = command_fifo[1];
	auto coord1 = command_fifo[2];
	auto point2 = command_fifo[3];
	auto coord2 = command_fifo[4];
	auto point3 = command_fifo[5];
	auto coord3 = command_fifo[6];
	auto point4 = command_fifo[7];
	auto coord4 = command_fifo[8];

	quad.point[0] = create_pixel(point1, color, coord1);
	quad.point[1] = create_pixel(point2, color, coord2);
	quad.point[2] = create_pixel(point3, color, coord3);
	quad.point[3] = create_pixel(point4, color, coord4);
	
	/* Set up the CLUT attribute for texel lookup. */
	ClutAttrib clut;
	clut.raw = command_fifo[2] >> 16;

	/* Find the correct texture page. */
	TPageAttrib page;
	page.raw = command_fifo[4] >> 16;

	quad.clut_x = clut.x * 16;
	quad.clut_y = clut.y;

	quad.base_u = page.page_x * 64;
	quad.base_v = page.page_y * 256;
	quad.depth = page.page_colors;

	raster.draw_polygon_textured(quad);
}

void GPU::gp0_shaded_trig()
{
	//printf("Draw Shaded Triangle!\n");
	
	auto color1 = command_fifo[0];
	auto point1 = command_fifo[1];
	auto color2 = command_fifo[2];
	auto point2 = command_fifo[3];
	auto color3 = command_fifo[4];
	auto point3 = command_fifo[5];

	auto v0 = create_pixel(point1, color1);
	auto v1 = create_pixel(point2, color2);
	auto v2 = create_pixel(point3, color3);

	Triangle tr = { v0, v1, v2 };

	raster.draw_polygon_shaded(tr);
}

void GPU::gp1_reset(uint32_t data)
{
	//printf("GPU GP1 reset\n");
	status.interrupt_req = false;

	status.raw = 0;
	texture_x_flip = false;
	texture_y_flip = false;
	
	drawing_area_left = 0;
	drawing_area_right = 0;
	drawing_area_top = 0;
	drawing_area_bottom = 0;
	drawing_x_offset = 0;
	drawing_y_offset = 0;
	
	display_vram_x_start = 0;
	display_vram_y_start = 0;
	display_horiz_start = 0x200;
	display_horiz_end = 0xc00;
	display_line_start = 0x10;
	display_line_end = 0x100;

	gp1_reset_cmd_buffer(data);
	gp1_acknowledge_irq(data);
}

void GPU::gp1_display_mode(uint32_t data)
{
	//printf("GPU GP1 display mode\n");
	uint8_t hr1 = bit_range(data, 0, 2);
	uint8_t hr2 = get_bit(data, 6);

	status.hres = (hr2 & 1) | ((hr1 & 3) << 1);
	status.vres = (VerticalRes)get_bit(data, 2);
	status.video_mode = (VideoMode)get_bit(data, 3);
	status.color_depth = (ColorDepth)get_bit(data, 4);
	status.vertical_interlace = get_bit(data, 5);

	if (get_bit(data, 7) != 0) {
		//printf("Unsupported display mode: 0x%x\n", data);
		exit(0);
	}
}

void GPU::gp1_dma_dir(uint32_t data)
{
	//printf("GPU GP1 dma dir\n");
	
	switch (data & 3) {
	case 0: 
		status.dma_dir = DMADirection::Off; 
		break;
	case 1: 
		status.dma_dir = DMADirection::Fifo; 
		break;
	case 2: 
		status.dma_dir = DMADirection::Cpu_Gpu; 
		break;
	case 3: 
		status.dma_dir = DMADirection::Vram_Cpu; 
		break;
	}
}

void GPU::gp1_horizontal_display_range(uint32_t data)
{
	//printf("GPU GP1 horizontal display range\n");
	display_horiz_start = bit_range(data, 0, 12);
	display_horiz_end = bit_range(data, 12, 24);
}

void GPU::gp1_vertical_display_range(uint32_t data)
{
	//printf("GPU GP1 vertical display range\n");
	display_line_start = (uint16_t)(data & 0x3ff);
	display_line_end = (uint16_t)((data >> 10) & 0x3ff);
}

// TODO change values
void GPU::gp1_start_of_display_area(uint32_t data)
{
	//printf("GPU GP1 start of display area\n");
	display_vram_x_start = bit_range(data, 0, 10);
	display_vram_y_start = bit_range(data, 10, 19);
}

void GPU::gp1_display_enable(uint32_t data)
{
	//printf("GPU GP1 displat enable\n");
	status.display_disable = get_bit(data, 0);
}

void GPU::gp1_acknowledge_irq(uint32_t data)
{
	//printf("GPU GP1 acknowledge interrupt\n");
	status.interrupt_req = false;
}

void GPU::gp1_reset_cmd_buffer(uint32_t data)
{
	//printf("GPU GP1 reset command buffer\n");
	command_fifo.clear();
	remaining_attribs = 0;
	gpu_mode = Command;
}

void GPU::gp1_gpu_info(uint32_t data)
{
	//printf("GPU GP1 info!\n");
	uint32_t info = data & 0xF;
	
	switch (info) {
	case 0x2: 
		read = (uint32_t)(texture_window_y_offset << 15 | texture_window_x_offset << 10 | texture_window_y_mask << 5 | texture_window_x_mask);
		break;
	case 0x3: 
		read = (uint32_t)(drawing_area_top << 10 | drawing_area_left);
		break;
	case 0x4: 
		read = (uint32_t)(drawing_area_bottom << 10 | drawing_area_right);
		break;
	case 0x5: 
		read = (uint32_t)(drawing_y_offset << 11 | (uint16_t)drawing_x_offset);
		break;
	case 0x7: 
		read = 2;
		break;
	case 0x8: 
		read = 0;
		break;
	default: 
		//printf("GPU GP1 unhandled get info: 0x%x\n", data); 
		break;
	}
}
