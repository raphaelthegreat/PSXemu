#include "gpu.h"
#include <cpu/util.h>
#include <cpu/cpu.h>
#include <video/renderer.h>
#include <Windows.h>
#include <gl/GLU.h>

/* GPU class implementation */
GPU::GPU(Renderer* renderer)
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

	dot_clock = 0;
	frame_count = 0;
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	gluOrtho2D(0.0, 1280, 960, 0.0);
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
	printf("GPURead!\n");
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

void GPU::tick(uint32_t cycles)
{
	uint32_t max_cycles = video_mode_timings();
	uint32_t max_lines = lines_per_frame();
	VerticalRes vres = (VerticalRes)status.vres;

	/* Add the cycles to GPU pixel clock (this is in pixels). */
	dot_clock += cycles;

	/* Get the number of generated scanlines. */
	uint32_t new_lines = dot_clock / max_cycles;

	/* No new scanlines where generated. */
	if (new_lines == 0) 
		return;

	dot_clock %= max_cycles;

	/* Add the new scanlines. */
	scanline += new_lines;

	/* We are still drawing the frame (not in vblank). */
	if (scanline < VBLANK_START) {
		if (vres == VerticalRes::V480 && status.vertical_interlace)
			status.odd_lines = (frame_count % 2) != 0;
		else
			status.odd_lines = (scanline % 2) != 0;
	}
	else {
		status.odd_lines = false;
	}

	/* We have finished drawing and in vblank. */
	if (scanline > max_lines) {
		scanline = 0;
		frame_count++;
		in_vblank = true;

		/* Draw finished scene. */
	}

	in_vblank = false;
	return;
}

bool GPU::is_vblank()
{
	return in_vblank;
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
			if (data_mover.y == data_mover.height) {
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

Color8 GPU::unpack(uint16_t color)
{
	uint8_t red, green, blue, alpha;

	/* Unpack bits. */
	red = ((color & 0x1F) / 31.0) * 255.999;
	green = (((color & 0x3E0) >> 5) / 31.0) * 255.999;
	blue = (((color & 0x7C00) >> 10) / 31.0) * 255.999;
	alpha = (color >> 15) * 255;

	return Color8(red, green, blue, alpha);
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
			printf("Unhandled GPU command: 0x%x\n", (uint32_t)command);
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
	default:
		printf("Unhandled GP1 command: 0x%x\n", (uint32_t)command);
		exit(0);
	}
}

/* execute a NOP command. */
void GPU::gp0_nop()
{
	printf("GPU Nop\n");
	return;
}

void GPU::gp0_mono_quad()
{
	printf("Draw Mono Quad\n");

	Color8 color = Color8::from_gpu(command_fifo[0]);
	Verts pos =
	{
		Pos2i::from_gpu(command_fifo[1]),
		Pos2i::from_gpu(command_fifo[2]),
		Pos2i::from_gpu(command_fifo[3]),
		Pos2i::from_gpu(command_fifo[4]),
	};

	Colors colors = { color, color, color, color };
	gl_renderer->push_quad(pos, colors);
}

void GPU::gp0_draw_mode()
{
	printf("GPU set draw mode\n");
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

void GPU::gp0_draw_area_top_left()
{
	printf("GPU draw area top left\n");
	uint32_t data = command_fifo[0];

	drawing_area_top = (uint16_t)bit_range(data, 10, 20);
	drawing_area_left = (uint16_t)bit_range(data, 0, 10);
}

void GPU::gp0_draw_area_bottom_right()
{
	printf("GPU draw area bottom right\n");
	uint32_t data = command_fifo[0];

	drawing_area_bottom = (uint16_t)bit_range(data, 10, 20);
	drawing_area_right = (uint16_t)bit_range(data, 0, 10);
}

void GPU::gp0_texture_window_setting()
{
	printf("GPU texture window setting\n");
	uint32_t data = command_fifo[0];

	texture_window_x_mask = bit_range(data, 0, 5);
	texture_window_y_mask = bit_range(data, 5, 10);
	
	texture_window_x_offset = bit_range(data, 10, 15);
	texture_window_y_offset = bit_range(data, 15, 20);
}

void GPU::gp0_drawing_offset()
{
	printf("GPU drawing offset\n");
	uint32_t data = command_fifo[0];
	
	uint16_t x = (uint16_t)(data & 0x7ff);
	uint16_t y = (uint16_t)((data >> 11) & 0x7ff);

	drawing_x_offset = ((int16_t)(x << 5)) >> 5;
	drawing_y_offset = ((int16_t)(y << 5)) >> 5;
	
	//gl_renderer->draw_scene();
	gl_renderer->set_draw_offset(drawing_x_offset, drawing_y_offset);
}

void GPU::gp0_mask_bit_setting()
{
	printf("GPU mask bit setting\n");
	uint32_t data = command_fifo[0];

	status.force_set_mask_bit = get_bit(data, 0);
	status.preserve_masked_pixels = get_bit(data, 1);
}

void GPU::gp0_clear_cache()
{
	printf("GPU clear cache\n");
}

void GPU::gp0_image_load()
{
	uint32_t res = command_fifo[2];
	uint32_t width = bit_range(res, 0, 16);
	uint32_t height = bit_range(res, 16, 32);

	/* Compute image surface. */
	uint32_t imgsize = width * height;
	/* Round to next even number. */
	imgsize = (imgsize + 1) & ~1;

	/* Set the remaining attributtes. */
	/* This basically means that the next (pixel_count / 2) */
	/* atrtibutes are pixels (pixels are batched and sent 2 at a time). */
	remaining_attribs = imgsize / 2;

	/* Unpack texture coords. */
	uint32_t source_coord = command_fifo[1];
	uint32_t coordx = bit_range(source_coord, 0, 16);
	uint32_t coordy = bit_range(source_coord, 16, 32);

	/* Is the image not 0 */
	if (remaining_attribs > 0) {
		/* Image x/y in vram and width/height. */
		data_mover.start_x = coordx;
		data_mover.start_y = coordy;
		data_mover.width = width;
		data_mover.height = height;
		data_mover.pixel_count = remaining_attribs;
		
		data_mover.x = 0;
		data_mover.y = 0;

		/* Activate data mover. */
		data_mover.active = true;
	}
	else {
		printf("GPU image size 0!\n");
		exit(0);
	}

	gpu_mode = VRAMLoad;
}

void GPU::gp0_image_store()
{
	uint32_t res = command_fifo[2];
	uint32_t width = bit_range(res, 0, 16);
	uint32_t height = bit_range(res, 16, 32);
	
	printf("Unhandled image store: %d %d\n", width, height);
}

void GPU::gp0_shaded_quad()
{
	printf("Draw Shaded Quad!\n");
	
	Verts pos =
	{
		Pos2i::from_gpu(command_fifo[1]),
		Pos2i::from_gpu(command_fifo[3]),
		Pos2i::from_gpu(command_fifo[5]),
		Pos2i::from_gpu(command_fifo[7]),
	};

	Colors colors =
	{
		Color8::from_gpu(command_fifo[0]),
		Color8::from_gpu(command_fifo[2]),
		Color8::from_gpu(command_fifo[4]),
		Color8::from_gpu(command_fifo[6]),
	};

	gl_renderer->push_quad(pos, colors);
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
	printf("Draw Shaded Quad with Blending!\n");

	uint32_t start_x, start_y, width, height;
	
	/* Build vertex data array. */
	std::vector<Pos2i> verts(4);
	for (int i = 0; i < 4; i++) {
		uint32_t vertex = command_fifo[2 * i + 1];
		verts[i] = Pos2i::from_gpu(vertex);
	}

	/* Build texture coords array. */
	std::vector<Pos2f> coords =
	{
		Pos2f(0.0f, 0.0f),
		Pos2f(1.0f, 0.0f),
		Pos2f(0.0f, 1.0f),
		Pos2f(1.0f, 1.0f)
	};

	/* Define bottom left corner of the quad. */
	start_x = verts[0].x;
	start_y = verts[0].y;

	/* Quad width and height. */
	width = verts[1].x - start_x;
	height = verts[3].y - start_y;

	ClutAttrib clut;
	TPageAttrib page;

	/* Get CLUT and Texture Page attributes. */
	clut.raw = command_fifo[2] >> 16;
	page.raw = command_fifo[4] >> 16;
	
	/* Texpage Attribute (Parameter for Textured-Polygons commands)
	0-8    Same as GP0(E1h).Bit0-8 (see there)
	9-10   Unused (does NOT change GP0(E1h).Bit9-10)
	11     Same as GP0(E1h).Bit11  (see there)
	12-13  Unused (does NOT change GP0(E1h).Bit12-13)
	14-15  Unused (should be 0)*/
	uint32_t tx = page.page_x * 64;
	uint32_t ty = page.page_y * 256;
	
	/* Clut Attribute (Color Lookup Table, aka Palette)
	(This attribute is relevant only for 4bit/8bit textures).
	0-5      X coordinate X/16  (ie. in 16-halfword steps)
	6-14     Y coordinate 0-511 (ie. in 1-line steps)
	15       Unknown/unused (should be 0)*/
	uint32_t cx = clut.x * 16;
	uint32_t cy = clut.y;

	/* Read CLUT table from VRAM. */
	for (int i = 0; i < 16; i++) {
		uint32_t entry = vram.read(cx + i, cy);
		clut_table.push_back(entry);
	}
	
	/* Loop through the pixels in the texture page. */
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width / 4; x++) {
			image[x * 4 + 0][y] = clut_table[vram.buffer[tx + x][ty + y].ll];
			image[x * 4 + 1][y] = clut_table[vram.buffer[tx + x][ty + y].ml];
			image[x * 4 + 2][y] = clut_table[vram.buffer[tx + x][ty + y].mr];
			image[x * 4 + 3][y] = clut_table[vram.buffer[tx + x][ty + y].rr];
			
			/* For each pixel in a batch of 4 pixels seperate rgb colors. */
			for (int k = 0; k < 4; k++) {
				uint32_t mask = 15 << 4 * k;
				uint32_t pixel = vram.buffer[tx + x][ty + y].raw;
				
				uint32_t index = (pixel & mask) >> 4 * k;
				uint32_t data = clut_table[index];
				
				Color8 color = unpack(data);
				color.alpha = 255;
				
				/* In opaque draw commands */
				/* color value 0x0000 means tranparent. */
				/* and 0x8000 means black */
				if (data == 0) {
					color.red = 0;
					color.green = 0;
					color.blue = 0;
					color.alpha = 0;
				}
				else if (data == 0x8000) {
					color.red = 0;
					color.green = 0;
					color.blue = 0;
				}

				/* Push data to pixel buffer. */
				pixels.push_back(color.red);
				pixels.push_back(color.green);
				pixels.push_back(color.blue);
				pixels.push_back(color.alpha);
			}
		}
	}
	
	/* Create TextureInfo object. */
	TextureInfo info;
	info.width = width;
	info.height = height;
	info.vram_x = start_x;
	info.vram_y = start_y;
	info.format = Format::RGBA;

	/* Update texture with generated pixels. */
	gl_renderer->update_texture(info, pixels);

	/* Clear tables after use. */
	clut_table.clear();
	pixels.clear();

	/* Push the quad to the renderer to be drawn. */
	gl_renderer->push_textured_quad(verts, coords, info);
}

void GPU::gp0_shaded_trig()
{
	printf("Draw Shaded Triangle!\n");

	Verts pos =
	{
		Pos2i::from_gpu(command_fifo[1]),
		Pos2i::from_gpu(command_fifo[3]),
		Pos2i::from_gpu(command_fifo[5])
	};

	Colors colors =
	{
		Color8::from_gpu(command_fifo[0]),
		Color8::from_gpu(command_fifo[2]),
		Color8::from_gpu(command_fifo[4])
	};

	gl_renderer->push_triangle(pos, colors);
}

void GPU::gp1_reset(uint32_t data)
{
	printf("GPU GP1 reset\n");
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
	printf("GPU GP1 display mode\n");
	uint8_t hr1 = bit_range(data, 0, 2);
	uint8_t hr2 = get_bit(data, 6);

	status.hres = (hr2 & 1) | ((hr1 & 3) << 1);
	status.vres = (VerticalRes)get_bit(data, 2);
	status.video_mode = (VideoMode)get_bit(data, 3);
	status.color_depth = (ColorDepth)get_bit(data, 4);
	status.vertical_interlace = get_bit(data, 5);

	if (get_bit(data, 7) != 0)
		panic("Unsupported display mode: 0x", data);

	gl_renderer->update();
}

void GPU::gp1_dma_dir(uint32_t data)
{
	printf("GPU GP1 dma dir\n");
	
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
	printf("GPU GP1 horizontal display range\n");
	display_horiz_start = (uint16_t)(data & 0xfff);
	display_horiz_end = (uint16_t)((data >> 12) & 0xfff);
}

void GPU::gp1_vertical_display_range(uint32_t data)
{
	printf("GPU GP1 vertical display range\n");
	display_line_start = (uint16_t)(data & 0x3ff);
	display_line_end = (uint16_t)((data >> 10) & 0x3ff);
}

// TODO change values
void GPU::gp1_start_of_display_area(uint32_t data)
{
	printf("GPU GP1 start of display area\n");
	display_vram_x_start = bit_range(data, 0, 10);
	display_vram_y_start = bit_range(data, 10, 19);
}

void GPU::gp1_display_enable(uint32_t data)
{
	printf("GPU GP1 displat enable\n");
	status.display_disable = get_bit(data, 0);
}

void GPU::gp1_acknowledge_irq(uint32_t data)
{
	printf("GPU GP1 acknowledge interrupt\n");
	status.interrupt_req = false;
}

void GPU::gp1_reset_cmd_buffer(uint32_t data)
{
	printf("GPU GP1 reset command buffer\n");
	command_fifo.clear();
	remaining_attribs = 0;
	gpu_mode = Command;
}
