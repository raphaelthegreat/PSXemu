#include "gpu.h"
#include <cpu/util.h>
#include <video/renderer.h>

HorizontalRes HorizontalRes::from_bits(uint8_t h1, uint8_t h2)
{
	HorizontalRes hr;
	hr.hr = (h2 & 1) | ((h1 & 3) << 1);

	return hr;
}


uint8_t VRAMLine::get4bit(uint32_t index)
{
	uint32_t i = std::floor(index / 2);
	uint32_t b = index % 2;

	return (data[i] & (0xf << 4 * b)) >> 4 * b;
}

uint8_t VRAMLine::get8bit(uint32_t index)
{
	uint32_t i = index;
	return data[i];
}

uint16_t VRAMLine::get16bit(uint32_t index)
{
	uint32_t i = index;
	return data[i] | (data[i + 1] << 8);
}

uint32_t VRAMLine::get24bit(uint32_t index)
{
	uint32_t i = std::floor(index / 3);
	/* TODO: this will overflow if index = 682. */
	return data[i] | (data[i + 1] << 8) | (data[i + 2] << 16);
}

/* GPU class implementation */

GPU::GPU(Renderer* renderer)
{
	gl_renderer = renderer;
	
	status.page_base_x = 0;
	status.page_base_y = 0;
	status.semi_transprency = 0;
	status.texture_depth = TextureDepth::D4bit;
	status.dithering = false;
	status.draw_to_display = false;
	status.force_set_mask_bit = false;
	status.preserve_masked_pixels = false;
	status.field = Field::Top;
	status.texture_disable = false;
	status.hres = HorizontalRes::from_bits(0, 0);
	status.vres = VerticalRes::V240;
	status.video_mode = VideoMode::NTSC;
	status.color_depth = ColorDepth::C15bit;
	status.vertical_interlace= false;
	status.display_disable = true;
	status.interrupt_req = false;
	image_load = false;
	status.dma_dir = DMADirection::Off;

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
	image_load = false;

	texture_window_x_mask = 0;
	texture_window_y_mask = 0;
	texture_window_x_offset = 0;
	texture_window_y_offset = 0;

	texture_x_flip = false;
	texture_y_flip = false;
}

uint32_t GPU::get_status()
{
	uint32_t r = 0;

	r |= (status.page_base_x) << 0;
	r |= (status.page_base_y) << 4;
	r |= (status.semi_transprency) << 5;
	r |= (uint32_t)(status.texture_depth) << 7;
	r |= (status.dithering) << 9;
	r |= (status.draw_to_display) << 10;
	r |= (status.force_set_mask_bit) << 11;
	r |= (status.preserve_masked_pixels) << 12;
	r |= (uint32_t)(status.field) << 13;
	// Bit 14: not supported
	r |= (status.texture_disable) << 15;
	r |= (uint32_t)(status.hres.hr) << 16;
	// XXX Temporary hack: if we don't emulate bit 31 correctly
	// setting `vres` to 1 locks the BIOS:
	// r |= (vres) << 19;
	r |= (uint32_t)(status.video_mode) << 20;
	r |= (uint32_t)(status.color_depth) << 21;
	r |= (status.vertical_interlace) << 22;
	r |= (status.display_disable) << 23;
	r |= (status.interrupt_req) << 24;

	// For now we pretend that the GPU is always ready:
	// Ready to receive command
	r |= 1 << 26;
	// Ready to send VRAM to CPU
	r |= 1 << 27;
	// Ready to receive DMA block
	r |= 1 << 28;

	r |= (uint32_t)(status.dma_dir) << 29;

	// Bit 31 should change depending on the currently drawn line
	// (whether it's even, odd or in the vblack apparently). Let's
	// not bother with it for now.
	r |= 0 << 31;

	// Not sure about that, I'm guessing that it's the signal
	// checked by the DMA in when sending data in Request
	// synchronization mode. For now I blindly follow the Nocash
	// spec.
	auto dma_request = 0;
		switch (status.dma_dir) {
		// Always 0
		case DMADirection::Off: dma_request = 0; break;
		// Should be 0 if FIFO is full, 1 otherwise
		case DMADirection::Fifo: dma_request = 1; break;
		// Should be the same as status bit 28
		case DMADirection::Cpu_Gpu: dma_request = (r >> 28) & 1; break;
		// Should be the same as status bit 27
		case DMADirection::Vram_Cpu: dma_request = (r >> 27) & 1; break;
	};

	r |= dma_request << 25;

	return r;
}

uint32_t GPU::get_read()
{
	printf("GPURead!");
	return 0;
}

void GPU::gp0_command(uint32_t data)
{
	/* Handle a new GPU command. */
	if (remaining_attribs == 0) {
		GP0Command command = (GP0Command)bit_range(data, 24, 32);

		std::pair<uint32_t, GP0Func> handler;

		/* Select the appropriate handler. */
		switch (command) {
		case GP0Command::Nop:
			handler = std::make_pair(1, BIND(gp0_nop));
			break;
		case GP0Command::Shaded_Quad:
			handler = std::make_pair(8, BIND(gp0_shaded_quad));
			break;
		case GP0Command::Shaded_Quad_Blend:
			handler = std::make_pair(9, BIND(gp0_shaded_quad_blend));
			break;
		case GP0Command::Shaded_Triangle:
			handler = std::make_pair(6, BIND(gp0_shaded_trig));
			break;
		case GP0Command::Image_Load:
			handler = std::make_pair(3, BIND(gp0_image_load));
			break;
		case GP0Command::Image_Store:
			handler = std::make_pair(3, BIND(gp0_image_store));
			break;
		case GP0Command::Clear_Cache:
			handler = std::make_pair(1, BIND(gp0_clear_cache));
			break;
		case GP0Command::Mono_Quad:
			handler = std::make_pair(5, BIND(gp0_mono_quad));
			break;
		case GP0Command::Draw_Area_Bottom_Right:
			handler = std::make_pair(1, BIND(gp0_draw_area_bottom_right));
			break;
		case GP0Command::Draw_Area_Top_Left:
			handler = std::make_pair(1, BIND(gp0_draw_area_top_left));
			break;
		case GP0Command::Drawing_Offset:
			handler = std::make_pair(1, BIND(gp0_drawing_offset));
			break;
		case GP0Command::Draw_Mode_Setting:
			handler = std::make_pair(1, BIND(gp0_draw_mode));
			break;
		case GP0Command::Texture_Window_Setting:
			handler = std::make_pair(1, BIND(gp0_texture_window_setting));
			break;
		case GP0Command::Mask_Bit_Setting:
			handler = std::make_pair(1, BIND(gp0_mask_bit_setting));
			break;
		default:
			printf("Unhandled GPU command: 0x%x\n", (uint32_t)command);
			exit(0);
		}

		remaining_attribs = handler.first;
		command_handler = handler.second;

		/* Prepare the fifo for a new set of data. */
		command_fifo.clear();
	}

	/* If we are here it means that we received command attribs. */
	remaining_attribs--;

	/* Push attrib to command fifo. */
	if (!image_load) {
		command_fifo.push_back(data);

		/* Once we gether everything we need, execute the command. */
		if (remaining_attribs == 0)
			command_handler();
	}
	else { /* Push pixel to texture buffer. */
		buffer.push_data(data);
		
		/* Push the final image to the renderer to be drawn. */
		if (remaining_attribs == 0) {
			gl_renderer->push_image(buffer);
			buffer.pixels.clear();
			image_load = false;
		}
	}
}

void GPU::gp1_command(uint32_t data)
{
	GP1Command command = (GP1Command)bit_range(data, 24, 32);

	if (command == GP1Command::Reset)
		return gp1_reset(data);
	else if (command == GP1Command::Acknowledge_Irq)
		return gp1_acknowledge_irq(data);
	else if (command == GP1Command::Display_Enable)
		return gp1_display_enable(data);
	else if (command == GP1Command::Reset_Cmd_Buffer)
		return gp1_reset_cmd_buffer(data);
	else if (command == GP1Command::DMA_Direction)
		return gp1_dma_dir(data);
	else if (command == GP1Command::Start_Of_Display_Area)
		return gp1_start_of_display_area(data);
	else if (command == GP1Command::Horizontal_Display_Range)
		return gp1_horizontal_display_range(data);
	else if (command == GP1Command::Vertical_Display_Range)
		return gp1_vertical_display_range(data);
	else if (command == GP1Command::Display_Mode)
		return gp1_display_mode(data);
	else
		panic("Unhandled GP1 command: 0x", (uint32_t)command);
}

void GPU::gp0_nop()
{
	printf("GPU Nop\n");
	return;
}

void GPU::gp0_mono_quad()
{
	printf("Draw Mono Quad\n");

	Color color = Color::from_gpu(command_fifo[0]);
	
	Verts pos =
	{
		Pos2::from_gpu(command_fifo[1]),
		Pos2::from_gpu(command_fifo[2]),
		Pos2::from_gpu(command_fifo[3]),
		Pos2::from_gpu(command_fifo[4]),
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
	status.texture_depth = (TextureDepth)bit_range(data, 7, 9);
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
	
	gl_renderer->update();
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

	printf("GPU load image w:%d  h:%d\n", width, height);

	uint32_t imgsize = width * height;
	imgsize = (imgsize + 1) & ~1;

	remaining_attribs = imgsize / 2;

	uint32_t source_coord = command_fifo[1];
	uint32_t coordx = bit_range(source_coord, 0, 16);
	uint32_t coordy = bit_range(source_coord, 16, 32);
	 
	/* Is the image not 0 */
	if (remaining_attribs > 0) {
		buffer.pixels = std::vector<uint16_t>(imgsize);
		buffer.width = width;
		buffer.height = height;
		buffer.top_left = std::make_pair(coordx, coordy);

		
	}
	else {
		printf("GPU image size 0!\n");
		exit(0);
	}

	image_load = true;
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
		Pos2::from_gpu(command_fifo[1]),
		Pos2::from_gpu(command_fifo[3]),
		Pos2::from_gpu(command_fifo[5]),
		Pos2::from_gpu(command_fifo[7]),
	};

	Colors colors =
	{
		Color::from_gpu(command_fifo[0]),
		Color::from_gpu(command_fifo[2]),
		Color::from_gpu(command_fifo[4]),
		Color::from_gpu(command_fifo[6]),
	};

	gl_renderer->push_quad(pos, colors);
}

void GPU::gp0_shaded_quad_blend()
{
	printf("Draw Shaded Quad with Blending!\n");

	Verts pos =
	{
		Pos2::from_gpu(command_fifo[1]),
		Pos2::from_gpu(command_fifo[3]),
		Pos2::from_gpu(command_fifo[5]),
		Pos2::from_gpu(command_fifo[7]),
	};
	
	Color color(0x80, 0x80, 0x80);
	Colors colors = { color, color, color, color };

	gl_renderer->push_quad(pos, colors);
}

void GPU::gp0_shaded_trig()
{
	printf("Draw Shaded Triangle!\n");

	Verts pos =
	{
		Pos2::from_gpu(command_fifo[1]),
		Pos2::from_gpu(command_fifo[3]),
		Pos2::from_gpu(command_fifo[5])
	};

	Colors colors =
	{
		Color::from_gpu(command_fifo[0]),
		Color::from_gpu(command_fifo[2]),
		Color::from_gpu(command_fifo[4])
	};

	gl_renderer->push_triangle(pos, colors);
}

void GPU::gp1_reset(uint32_t data)
{
	printf("GPU GP1 reset\n");
	status.interrupt_req = false;

	status.page_base_x = 0;
	status.page_base_y = 0;
	status.semi_transprency = 0;
	status.texture_depth = TextureDepth::D4bit;
	texture_window_x_mask = 0;
	texture_window_y_mask = 0;
	texture_window_x_offset = 0;
	texture_window_y_offset = 0;
	status.dithering = false;
	status.draw_to_display = false;
	status.texture_disable = false;
	texture_x_flip = false;
	texture_y_flip = false;
	drawing_area_left = 0;
	drawing_area_right = 0;
	drawing_area_top = 0;
	drawing_area_bottom = 0;
	drawing_x_offset = 0;
	drawing_y_offset = 0;
	status.force_set_mask_bit = false;
	status.preserve_masked_pixels = false;

	status.dma_dir = DMADirection::Off;

	status.display_disable = true;
	display_vram_x_start = 0;
	display_vram_y_start = 0;
	status.hres = HorizontalRes::from_bits(0, 0);
	status.vres = VerticalRes::V240;

	// XXX does PAL hardware reset to this config as well?
	status.video_mode = VideoMode::NTSC;
	status.vertical_interlace = true;
	display_horiz_start = 0x200;
	display_horiz_end = 0xc00;
	display_line_start = 0x10;
	display_line_end = 0x100;
	status.color_depth = ColorDepth::C15bit;

	gp1_reset_cmd_buffer(data);
	gp1_acknowledge_irq(data);
}

void GPU::gp1_display_mode(uint32_t data)
{
	printf("GPU GP1 display mode\n");
	uint8_t hr1 = bit_range(data, 0, 2);
	uint8_t hr2 = get_bit(data, 6);

	status.hres = HorizontalRes::from_bits(hr1, hr2);
	status.vres = (VerticalRes)get_bit(data, 2);
	status.video_mode = (VideoMode)get_bit(data, 3);
	status.color_depth = (ColorDepth)get_bit(data, 4);
	status.vertical_interlace = get_bit(data, 5);

	if (get_bit(data, 7) != 0)
		panic("Unsupported display mode: 0x", data);
}

void GPU::gp1_dma_dir(uint32_t data)
{
	printf("GPU GP1 dma dir\n");
	switch (data & 3) {
	case 0: status.dma_dir = DMADirection::Off; break;
	case 1: status.dma_dir = DMADirection::Fifo; break;
	case 2: status.dma_dir = DMADirection::Cpu_Gpu; break;
	case 3: status.dma_dir = DMADirection::Vram_Cpu; break;
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
	image_load = false;

	// TODO : clear the command FIFO
}
