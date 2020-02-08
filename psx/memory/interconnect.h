#pragma once
#include <cstdio>
#include <optional>
#include <memory>

#include <memory/bios.h>
#include <memory/ram.h>
#include <memory/dma.h>
#include <video/gpu.h>
#include <cpu/util.h>
using std::unique_ptr;

class Range {
public:
	Range(uint32_t st, uint32_t len) : 
		start(st), length(len) {}

	std::optional<uint32_t> contains(uint32_t addr) const;

public:
	uint32_t offset = 0;

private:
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

class Interconnect {
public:
	Interconnect(std::string bios_path, Renderer* renderer);
	~Interconnect() = default;

	template <typename T = uint32_t>
	T read(uint32_t addr);

	template <typename T = uint32_t>
	void write(uint32_t addr, T data);

	uint32_t mask_region(uint32_t addr);

public:
	unique_ptr<Bios> bios;
	unique_ptr<Ram> ram;

	DMAController dma;
	unique_ptr<GPU> gpu;

	Renderer* gl_renderer;

	const uint32_t region_mask[8] = {
		0xffffffff, 0xffffffff,
		0xffffffff, 0xffffffff,
		0x7fffffff, 0x1fffffff,
		0xffffffff, 0xffffffff,
	};
};

template<typename T>
inline T Interconnect::read(uint32_t addr)
{
	if (addr % sizeof(T) != 0)
		panic("Unaligned read at address: 0x", addr);

	uint32_t abs_addr = mask_region(addr);

	if (auto offset = EXPANSION_1.contains(abs_addr); offset.has_value())
		return 0xff;
	if (auto offset = SPU.contains(abs_addr); offset.has_value()) {
		printf("Unhandled read from SPU: 0x%x\n", addr);
		return 0;
	}
	if (auto offset = GPU_RANGE.contains(abs_addr); offset.has_value()) {
		printf("GPU read at address: 0x%x\n", addr);

		if (offset.value() == 0)
			return gpu->get_read();
		else if (offset.value() == 4)
			return gpu->get_status();
		else
			panic("Unhandled GPU read at offset 0x", offset.value());
	}
	if (auto offset = TIMERS.contains(abs_addr); offset.has_value()) {
		printf("Timer read at address: 0x%x\n", addr);
		return 0;
	}
	if (auto offset = DMA.contains(abs_addr); offset.has_value()) {
		printf("DMA read at address: 0x%x\n", addr);
		return dma.read(offset.value());
	}
	if (auto offset = BIOS.contains(abs_addr); offset.has_value())
		return bios->read<T>(offset.value());
	if (auto offset = RAM.contains(abs_addr); offset.has_value())
		return ram->read<T>(offset.value());
	if (auto offset = IRQ_CONTROL.contains(abs_addr); offset.has_value()) {
		printf("IRQ control read: 0x%x\n", addr);
		return 0;
	}

	panic("Unhandled read to address: 0x", addr);
	return 0;
}

template<typename T>
inline void Interconnect::write(uint32_t addr, T data)
{
	if (addr % sizeof(T) != 0)
		panic("Unaligned 32bit write to address: 0x", addr);

	uint32_t abs_addr = mask_region(addr);

	if (auto offset = TIMERS.contains(abs_addr); offset.has_value()) { // Ignore Timer write
		printf("Unhandled timer write to address: 0x%x\n", addr);
		return;
	}
	else if (auto offset = EXPANSION_2.contains(abs_addr); offset.has_value()) // Ignore expansion 2 writes
		return;
	if (auto offset = SPU.contains(abs_addr); offset.has_value()) // Ignore SPU write
		return;
	if (auto offset = GPU_RANGE.contains(abs_addr); offset.has_value()) {
		printf("GPU write to address: 0x%x with data 0x%x\n", addr, data);

		if (offset.value() == 0)
			return gpu->write_gp0(data);
		else if (offset.value() == 4)
			return gpu->write_gp1(data);
		else
			panic("Unahandled GPU write at offset: 0x", offset.value());
	}
	if (auto offset = DMA.contains(abs_addr); offset.has_value()) {
		printf("DMA write to address: 0x%x with data 0x%x\n", addr, data);
		return dma.write(offset.value(), data);
	}
	if (auto offset = RAM.contains(abs_addr); offset.has_value())
		return ram->write<T>(offset.value(), data);
	else if (auto offset = CACHE_CONTROL.contains(abs_addr); offset.has_value()) // CACHE CONTROL register
		return;
	else if (auto offset = RAM_SIZE.contains(abs_addr); offset.has_value()) // RAM_SIZE register
		return;
	else if (auto offset = IRQ_CONTROL.contains(abs_addr); offset.has_value()) {
		printf("Unahandled IRQ control write to address: 0x%x\n", addr);
		return;
	}
	else if (auto offset = SYS_CONTROL.contains(abs_addr); offset.has_value()) { // Expansion register
		if (offset.value() == 0 && data != 0x1f000000)
			panic("Attempt to remap expansion 1: 0x", data);
		else if (offset.value() == 4 && data != 0x1f802000)
			panic("Attempt to remap expansion 1: 0x", data);
		return;
	}

	panic("Unhandled write to address: 0x", addr);
}
