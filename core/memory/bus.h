#pragma once
#include <memory/bios.h>
#include <memory/range.h>
#include <memory/scratchpad.h>
#include <memory/ram.h>
#include <memory/dma.h>

#include <devices/cdrom_drive.hpp>
#include <devices/timer.h>
#include <devices/controller.h>

#include <cpu/cache.h>
#include <video/gpu_core.h>
using std::unique_ptr;

enum class ExceptionType {
	Interrupt = 0x0,
	ReadError = 0x4,
	WriteError = 0x5,
	BusError = 0x6,
	Syscall = 0x8,
	Break = 0x9,
	IllegalInstr = 0xA,
	CoprocessorError = 0xB,
	Overflow = 0xC
};

class CPU;
class Renderer;
class Bus {
public:
	Bus(std::string bios_path, Renderer* renderer);
	~Bus() = default;

	void tick();
	std::tuple<uint, uint, uint, uint> loadEXE(std::string test);

	template <typename T = uint>
	T read(uint addr);

	template <typename T = uint>
	void write(uint addr, T data);

	void irq(Interrupt interrupt);
	uint physical_addr(uint addr);

public:
	unique_ptr<Bios> bios;
	unique_ptr<Ram> ram;

	/* Peripherals. */
	DMAController dma;
	ControllerManager controller;
	CacheControl cache_ctrl;
	Scratchpad scratchpad;
	CDManager cddrive;

	Renderer* gl_renderer;
	GPU* gpu;
	CPU* cpu;

	uint spu_delay = 0;
	ubyte registers[4 * 1024] = {};
	ubyte ex1[512 * 1024] = {};

	Timer timers[3] = {
		{ TimerID::TMR0, this },
		{ TimerID::TMR1, this },
		{ TimerID::TMR2, this }
	};
	
	uint ram_size = 0;

	const uint region_mask[8] = {
		0xffffffff, 0xffffffff,
		0xffffffff, 0xffffffff,
		0x7fffffff, 0x1fffffff,
		0xffffffff, 0xffffffff,
	};
};
