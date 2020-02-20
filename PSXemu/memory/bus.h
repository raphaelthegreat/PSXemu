#pragma once
#include <cstdio>
#include <optional>
#include <memory>

#include <memory/bios.h>
#include <memory/ram.h>
#include <memory/dma.h>
#include <video/gpu.h>
#include <cpu/cache.h>
#include <devices/irq.h>
#include <cpu/util.h>
using std::unique_ptr;

class Range {
public:
	Range(uint32_t st, uint32_t len) : 
		start(st), length(len) {}

	std::optional<uint32_t> contains(uint32_t addr) const;

public:
	uint32_t start, length;
};

class CPU;
class Bus {
public:
	Bus(std::string bios_path, Renderer* renderer);
	~Bus() = default;

	template <typename T = uint32_t>
	T read(uint32_t addr);

	template <typename T = uint32_t>
	void write(uint32_t addr, T data);

	uint32_t physical_addr(uint32_t addr);

public:
	unique_ptr<Bios> bios;
	unique_ptr<Ram> ram;

	DMAController dma;
	InterruptController interruptController;
	CacheControl cache_ctrl;

	Renderer* gl_renderer;
	CPU* cpu;
	GPU* gpu;

	const uint32_t region_mask[8] = {
		0xffffffff, 0xffffffff,
		0xffffffff, 0xffffffff,
		0x7fffffff, 0x1fffffff,
		0xffffffff, 0xffffffff,
	};
};
