#include <stdafx.hpp>
#include "bus.h"
#include <video/vram.h>
#include <glad/glad.h>
#include <video/renderer.h>
#include <cpu/cpu.h>
#include <sound/spu.hpp>
#include <memory/expansion2.hpp>

std::vector<ubyte> load_file(fs::path const& filepath) 
{
	std::ifstream ifs(filepath, std::ios::binary | std::ios::ate | std::ios::in);

	if (!ifs)
		throw std::runtime_error(filepath.string() + ": " + std::strerror(errno));

	const auto size = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	if (size == 0) {
		__debugbreak();
		return {};
	}

	std::vector<ubyte> buf(size);

	if (!ifs.read((char*)buf.data(), buf.size()))
		throw std::runtime_error(filepath.string() + ": " + std::strerror(errno));

	return buf;
}

Bus::Bus(const std::string& bios_path)
{
	/* Construct components. */
	renderer = std::make_unique<Renderer>(640, 480, "Playstation 1 emulator", this);
	cpu = std::make_shared<CPU>(this);
	gpu = std::make_unique<GPU>(renderer.get());
	spu = std::make_shared<SPU>(this);
	exp2 = std::make_shared<Expansion2>(this);

	/* Construct peripherals. */
	timers[0] = std::make_unique<Timer>(TimerID::TMR0, this);
	timers[1] = std::make_unique<Timer>(TimerID::TMR1, this);
	timers[2] = std::make_unique<Timer>(TimerID::TMR2, this);
	dma = std::make_unique<DMAController>(this);
	controller = std::make_unique<ControllerManager>(this);
	cddrive = std::make_unique<CDManager>(this);

	/* Construct debugging tools. */
	debugger = std::make_unique<Debugger>(this);
	debugger->push_widget<CPUWidget>();
	debugger->push_widget<MemWidget>();

	/* Open BIOS file. */
	util::read_binary_file(bios_path, 512 * 1024, bios);

	/* Configure window. */
	glfwSetWindowUserPointer(renderer->window, this);
	glfwSetKeyCallback(renderer->window, &Bus::key_callback);
}

/* Get the physical memory address from the virtual one. */
uint Bus::physical_addr(uint addr)
{
    uint index = addr >> 29;
    return (addr & region_mask[index]);
}

/* Used for keyboard input detection. */
/* TODO: Add mappable keyboard configurations. */
void Bus::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Bus* bus = (Bus*)glfwGetWindowUserPointer(window);

	/* Button was just pressed. */
	if (action == GLFW_PRESS) {
		/* Handle controller input. */
		bus->controller->controller.key_down(key);
		
		/* Enable wireframe mode. */
		if (key == GLFW_KEY_F1) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glClear(GL_COLOR_BUFFER_BIT);
		} /* Enable normal fill mode. */
		else if (key == GLFW_KEY_F2) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glClear(GL_COLOR_BUFFER_BIT);
		} /* Dump vram to image file. */
		else if (key == GLFW_KEY_F3) {
			vram.write_to_image();
		}/* Toggle debugging utilities. */
		else if (key == GLFW_KEY_F4) {
			bus->debug_enable = !bus->debug_enable;
		}
	} /* Button was just released. */
	else if (action == GLFW_RELEASE) {
		bus->controller->controller.key_up(key);
	}
}

/* Load a Playstation Executable file. */
/* NOTE: To execute it, the force_test function in the CPU must be called! */
/* TODO: Have a clean API for loading .bin/.cue and .exe files. */
bool Bus::loadEXE(std::string m_psxexe_path, PSEXELoadInfo& out_psx_load_info)
{
	if (m_psxexe_path.empty())
		return false;

	const std::vector<ubyte>& psx_exe_buf = load_file(m_psxexe_path);

	if (psx_exe_buf.empty())
		return false;

	struct PSXEXEHeader {
		char magic[8];  // "PS-X EXE"
		ubyte pad0[8];
		uint pc;         // initial PC
		uint r28;        // initial R28
		uint load_addr;  // destination address in RAM
		uint filesize;   // excluding header & must be N*0x800
		uint unk0[2];
		uint memfill_start;
		uint memfill_size;
		uint r29_r30;         // initial r29 and r30 base
		uint r29_r30_offset;  // initial r29 and r30 offset, added to above
		// etc, we don't care about anything else
	};

	const auto psx_exe = (PSXEXEHeader*)psx_exe_buf.data();

	if (std::strcmp(&psx_exe->magic[0], "PS-X EXE")) {
		printf("Not a valid PS-X EXE file!");
		exit(0);
		return false;
	}

	// We don't support memfill
	assert(psx_exe->memfill_start == 0);
	assert(psx_exe->memfill_size == 0);

	out_psx_load_info.pc = psx_exe->pc;
	out_psx_load_info.r28 = psx_exe->r28;
	out_psx_load_info.r29 = psx_exe->r29_r30;
	out_psx_load_info.r30 = psx_exe->r29_r30 + psx_exe->r29_r30_offset;

	constexpr auto PSXEXE_HEADER_SIZE = 0x800;

	const auto copy_src_begin = psx_exe_buf.data() + PSXEXE_HEADER_SIZE;
	const auto copy_src_end = copy_src_begin + psx_exe->filesize;
	const auto copy_dest_begin = ram + (psx_exe->load_addr & 0x7FFFFFFF);

	std::copy(copy_src_begin, copy_src_end, copy_dest_begin);

	return true;
}

/* Tick the major components. */
void Bus::tick()
{
	/* Tick the CPU. */
	for (int i = 0; i < 100; i++) {
		cpu->tick();
	}

	/* Handle requested interrupts. */
	cpu->handle_interrupts();

	/* Tick the peripherals. */
	dma->tick();
	controller->tick();
	cddrive->tick();

	/* Tick the GPU. */
	/* NOTE: The function returns a bool indicating */
	/* if it is Vblank or not. So anything in the brackets */
	/* will only be executed in Vblank. */
	if (gpu->tick(300)) {
		/* Display draw data. */
		renderer->update();

		/* Show debug utilities. */
		if (debug_enable) {
			debugger->display();
		}

		/* Swap back and front buffers. */
		renderer->swap();

		/* Publish VBLANK irq. */
		this->irq(Interrupt::VBLANK);
	}

	/* Sync the timers with the GPU */
	GPUSync sync = gpu->get_blanks_and_dot();
	/* Tick timers. */
	for (int i = 0; i < 2; i++) {
		timers[i]->tick(300);
		timers[i]->gpu_sync(sync);
	}

	/* NOTE: Timer 2 does not sync with the GPU! */
	timers[2]->tick(300);
}

/* Trigger an interrupt. */
void Bus::irq(Interrupt interrupt) const
{
	cpu->trigger(interrupt);
}

/* Read memory at address. */
template<typename T>
T Bus::read(uint addr)
{
	/* Map the memory ranges. */
	uint abs_addr = physical_addr(addr);

	/* HACK: Skip reading SPU_DELAY. */
	if (abs_addr == 0x1F801014) return 0;

	if (TIMERS.contains(abs_addr)) {
		ubyte timer = (abs_addr >> 4) & 3;
		return timers[timer]->read(abs_addr);
	}
	else if (RAM.contains(abs_addr)) {
		int offset = RAM.offset(abs_addr);
		return util::read_memory<T>(ram, offset);
	}
	else if (BIOS.contains(abs_addr)) {
		int offset = BIOS.offset(abs_addr);
		return util::read_memory<T>(bios, offset);
	}
	else if (SCRATCHPAD.contains(abs_addr)) {
		int offset = SCRATCHPAD.offset(abs_addr);
		return util::read_memory<T>(scratchpad, offset);
	}
	else if (EXPANSION_1.contains(abs_addr)) {
		return 0xff;
	}
	else if (CDROM.contains(abs_addr)) {
		if (std::is_same<T, ubyte>::value)
			return cddrive->read(abs_addr);
	}
	else if (GPU_RANGE.contains(abs_addr)) {
		return gpu->read(abs_addr);
	}
	else if (PAD_MEMCARD.contains(abs_addr)) {
		return controller->read<T>(abs_addr);
	}
	else if (DMA_RANGE.contains(abs_addr)) {
		return dma->read(abs_addr);
	}
	else if (SPU_RANGE.contains(abs_addr)) {
		return spu->read<T>(abs_addr);
	}
	else if (RAM_SIZE.contains(abs_addr)) {
		return 0x00000888;
	}
	else if (INTERRUPT.contains(abs_addr)) {
		return cpu->read_irq(abs_addr);
	}
	else {
		printf("[MEM] Emulator::read: unhandled read to address: 0x%x with width %d.\n", abs_addr, sizeof(T));
		return 0xFFFFFFFF;
	}
}

/* Write to memory at address with a specific value. */
template<typename T>
void Bus::write(uint addr, T value)
{
	/* Map the memory ranges. */
	uint abs_addr = physical_addr(addr);
	if (TIMERS.contains(abs_addr)) {
		ubyte timer = (abs_addr >> 4) & 3;
		return timers[timer]->write(abs_addr, (uint)value);
	}
	else if (GPU_RANGE.contains(abs_addr)) {
		return gpu->write(abs_addr, (uint)value);
	}
	else if (PAD_MEMCARD.contains(abs_addr)) {
		return controller->write<T>(abs_addr, value);
	}
	else if (SCRATCHPAD.contains(abs_addr)) {
		int offset = SCRATCHPAD.offset(abs_addr);
		return util::write_memory(scratchpad, offset, value);
	}
	else if (CDROM.contains(abs_addr)) {
		if (std::is_same<T, ubyte>::value)
			return cddrive->write(abs_addr, (ubyte)value);
	}
	else if (DMA_RANGE.contains(abs_addr)) {
		return dma->write(abs_addr, (uint)value);
	}
	else if (SPU_RANGE.contains(abs_addr)) {
		return spu->write<T>(abs_addr, value);
	}
	else if (RAM.contains(abs_addr)) {
		int offset = RAM.offset(abs_addr);
		return util::write_memory<T>(ram, offset, value);
	}
	else if (CACHE_CONTROL.contains(abs_addr)) {
		int offset = CACHE_CONTROL.offset(abs_addr);
		return util::write_memory<T>(&cpu->cache_control.value, offset, value);
	}
	else if (RAM_SIZE.contains(abs_addr)) {
		return;
	}
	else if (EXPANSION_2.contains(abs_addr)) {
		return exp2->write<T>(abs_addr, value);
	}
	else if (SYS_CONTROL.contains(abs_addr)) {
		return;
	}
	else if (INTERRUPT.contains(abs_addr)) {
		return cpu->write_irq(abs_addr, value);
	}
	else {
		printf("[MEM] Emulator::write: unhandled write to address: 0x%x with width %d\n", abs_addr, sizeof(T));
		return;
	}
}

/* Template definitions. */
template uint Bus::read<uint>(uint);
template ushort Bus::read<ushort>(uint);
template ubyte Bus::read<ubyte>(uint);

template void Bus::write<uint>(uint, uint);
template void Bus::write<ushort>(uint, ushort);
template void Bus::write<ubyte>(uint, ubyte);