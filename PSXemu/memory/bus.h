#pragma once
#include <cstdio>
#include <optional>
#include <memory>

#include <devices/timer.h>
#include <memory/bios.h>
#include <memory/ram.h>
#include <memory/dma.h>
#include <video/gpu.h>
#include <cpu/util.h>
#include <cpu/cache.h>
using std::unique_ptr;

class Range {
public:
	Range(uint32_t st, uint32_t len) : 
		start(st), length(len) {}

	std::optional<uint32_t> contains(uint32_t addr) const;

public:
	uint32_t start, length;
};

const Range RAM = Range(0x00000000, 2048 * 1024);
const Range BIOS = Range(0x1fc00000, 512 * 1024);
const Range SYS_CONTROL = Range(0x1f801000, 36);
const Range RAM_SIZE = Range(0x1f801060, 4);
const Range CACHE_CONTROL = Range(0xfffe0130, 4);
const Range SPU = Range(0x1f801c00, 640);
const Range EXPANSION_2 = Range(0x1f802000, 66);
const Range EXPANSION_1 = Range(0x1f000000, 512 * 1024);
const Range IRQ_CONTROL = Range(0x1f801070, 8);
const Range TIMERS = Range(0x1f801100, 0x30);
const Range DMA = Range(0x1f801080, 0x80);
const Range GPU_RANGE = Range(0x1f801810, 8);

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
	unique_ptr<GPU> gpu;

	DMAController dma;
	CacheControl cache_ctrl;
	Timer timers[3] = {};

	Renderer* gl_renderer;

	const uint32_t region_mask[8] = {
		0xffffffff, 0xffffffff,
		0xffffffff, 0xffffffff,
		0x7fffffff, 0x1fffffff,
		0xffffffff, 0xffffffff,
	};
};
