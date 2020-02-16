#include "bus.h"
#include <video/renderer.h>

const Range RAM = Range(0x00000000, 2048 * 1024);
const Range BIOS = Range(0x1fc00000, 512 * 1024);
const Range SYS_CONTROL = Range(0x1f801000, 36);
const Range RAM_SIZE = Range(0x1f801060, 4);
const Range CACHE_CONTROL = Range(0xfffe0130, 4);
const Range SPU_RANGE = Range(0x1f801c00, 640);
const Range EXPANSION_2 = Range(0x1f802000, 66);
const Range EXPANSION_1 = Range(0x1f000000, 512 * 1024);
const Range TIMERS = Range(0x1f801100, 0x30);
const Range DMA_RANGE = Range(0x1f801080, 0x80);
const Range GPU_RANGE = Range(0x1f801810, 8);

std::optional<uint32_t> Range::contains(uint32_t addr) const
{
    if (addr >= start && addr < start + length) {
        return addr - start;
    }
    
    return std::nullopt;
}

Bus::Bus(std::string bios_path, Renderer* renderer) :
	dma(this)
{
    gl_renderer = renderer;
    
	/* Construct memory regions. */
    bios = std::make_unique<Bios>(bios_path);
    ram = std::make_unique<Ram>();
}

uint32_t Bus::physical_addr(uint32_t addr)
{
    uint32_t index = addr >> 29;
    return (addr & region_mask[index]);
}

template<typename T>
T Bus::read(uint32_t addr)
{
	if (addr % sizeof(T) != 0)
		panic("Unaligned read at address: 0x", addr);

	uint32_t abs_addr = physical_addr(addr);

	if (auto offset = EXPANSION_1.contains(abs_addr); offset.has_value())
		return 0xff;
	if (auto offset = SPU_RANGE.contains(abs_addr); offset.has_value()) {
		//printf("Unhandled read from SPU: 0x%x\n", addr);
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
		return 0;
	}
	if (auto offset = DMA_RANGE.contains(abs_addr); offset.has_value()) {
		printf("DMA read at address: 0x%x\n", addr);
		return dma.read(offset.value());
	}
	if (auto offset = BIOS.contains(abs_addr); offset.has_value())
		return bios->read<T>(offset.value());
	if (auto offset = RAM.contains(abs_addr); offset.has_value())
		return ram->read<T>(offset.value());
	if (addr == 0x1F801070)
		return interruptController.loadISTAT();
	if (addr == 0x1F801074)
		return interruptController.loadIMASK();

	panic("Unhandled read to address: 0x", abs_addr);
	return 0;
}

template<typename T>
void Bus::write(uint32_t addr, T data)
{
	if (addr % sizeof(T) != 0) {
		printf("Unaligned write to address: 0x%x\n", addr);
		exit(0);
	}

	uint32_t abs_addr = physical_addr(addr);

	if (auto offset = TIMERS.contains(abs_addr); offset.has_value()) { // Ignore Timer write
		return;
	}
	else if (auto offset = EXPANSION_2.contains(abs_addr); offset.has_value()) // Ignore expansion 2 writes
		return;
	if (auto offset = SPU_RANGE.contains(abs_addr); offset.has_value()) // Ignore SPU write
		return;
	if (auto offset = GPU_RANGE.contains(abs_addr); offset.has_value()) {
		printf("GPU write to address: 0x%x with data 0x%x\n", addr, data);

		if (offset.value() == 0)
			return gpu->gp0_command(data);
		else if (offset.value() == 4)
			return gpu->gp1_command(data);
		else
			panic("Unahandled GPU write at offset: 0x", offset.value());
	}
	if (auto offset = DMA_RANGE.contains(abs_addr); offset.has_value()) {
		printf("DMA write to address: 0x%x with data 0x%x\n", addr, data);
		return dma.write(offset.value(), data);
	}
	if (auto offset = RAM.contains(abs_addr); offset.has_value())
		return ram->write<T>(offset.value(), data);
	else if (auto offset = CACHE_CONTROL.contains(abs_addr); offset.has_value()) {
		return;
	}
	else if (auto offset = RAM_SIZE.contains(abs_addr); offset.has_value()) // RAM_SIZE register
		return;
	else if (addr == 0x1F801070)
		return interruptController.writeISTAT(data);
	else if (addr == 0x1F801074)
		return interruptController.writeIMASK(data);
	else if (auto offset = SYS_CONTROL.contains(abs_addr); offset.has_value()) { // Expansion register
		if (offset.value() == 0 && data != 0x1f000000)
			panic("Attempt to remap expansion 1: 0x", data);
		else if (offset.value() == 4 && data != 0x1f802000)
			panic("Attempt to remap expansion 1: 0x", data);
		return;
	}

	panic("Unhandled write to address: 0x", addr);
}

/* Template definitions */
template uint32_t Bus::read<uint32_t>(uint32_t);
template uint16_t Bus::read<uint16_t>(uint32_t);
template uint8_t Bus::read<uint8_t>(uint32_t);

template void Bus::write<uint32_t>(uint32_t, uint32_t);
template void Bus::write<uint16_t>(uint32_t, uint16_t);
template void Bus::write<uint8_t>(uint32_t, uint8_t);
