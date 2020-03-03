#pragma once
#include <cstdio>
#include <optional>
#include <memory>

#include <memory/bios.h>
#include <memory/ram.h>
#include <memory/dma.h>

#include <devices/cdrom.h>
#include <devices/timer.h>

#include <cpu/enum.h>
#include <cpu/cache.h>
#include <cpu/util.h>

#include <video/gpu_core.h>
using std::unique_ptr;

struct Range {
	Range(uint32_t begin, uint32_t size) : 
		start(begin), length(size) {}

	inline bool contains(uint32_t addr) const;
	inline uint32_t offset(uint32_t addr) const;

public:
	uint32_t start, length;
};

class CPU;
class Renderer;
class Bus {
public:
	Bus(std::string bios_path, Renderer* renderer);
	~Bus() = default;

	void tick();

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
	CacheControl cache_ctrl;
	CDRom cdrom;

	Renderer* gl_renderer;
	GPU* gpu;
	CPU* cpu;

	Timer timers[3];
	const uint32_t region_mask[8] = {
		0xffffffff, 0xffffffff,
		0xffffffff, 0xffffffff,
		0x7fffffff, 0x1fffffff,
		0xffffffff, 0xffffffff,
	};
};
