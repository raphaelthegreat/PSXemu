#pragma once
#include <cstdio>
#include <optional>
#include <memory>

#include <memory/bios.h>
#include <memory/range.h>
#include <memory/scratchpad.h>
#include <memory/ram.h>
#include <memory/dma.h>

#include <devices/cdrom_drive.hpp>
#include <devices/timer.h>
#include <devices/controller.h>

#include <cpu/enum.h>
#include <cpu/cache.h>
#include <cpu/util.h>

#include <video/gpu_core.h>
using std::unique_ptr;

union Array {
	uint8_t byte[4 * 1024];
	uint16_t half[2 * 1024];
	uint32_t word[1024];
};

class CPU;
class Renderer;
class Bus {
public:
	Bus(std::string bios_path, Renderer* renderer);
	~Bus() = default;

	void tick();
	std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> loadEXE(std::string test);

	template <typename T = uint32_t>
	T read(uint32_t addr);

	template <typename T = uint32_t>
	void write(uint32_t addr, T data);

	void irq(Interrupt interrupt);
	uint32_t physical_addr(uint32_t addr);

public:
	unique_ptr<Bios> bios;
	unique_ptr<Ram> ram;

	/* Peripherals. */
	DMAController dma;
	JOYPAD controller;
	CacheControl cache_ctrl;
	Scratchpad scratchpad;
	io::CdromDrive cddrive;

	Renderer* gl_renderer;
	GPU* gpu;
	CPU* cpu;

	uint32_t spu_delay = 0;
	Array registers;

	TIMERS timers;
	
	const uint32_t region_mask[8] = {
		0xffffffff, 0xffffffff,
		0xffffffff, 0xffffffff,
		0x7fffffff, 0x1fffffff,
		0xffffffff, 0xffffffff,
	};
};
