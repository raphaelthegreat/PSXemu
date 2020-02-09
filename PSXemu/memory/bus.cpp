#include "bus.h"
#include <video/renderer.h>

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
    
    bios = std::make_unique<Bios>(bios_path);
    ram = std::make_unique<Ram>();
    gpu = std::make_unique<GPU>(gl_renderer);
}

uint32_t Bus::mask_region(uint32_t addr)
{
    uint32_t index = addr >> 29;
    return (addr & region_mask[index]);
}

template<typename T>
T Bus::read(uint32_t addr)
{
	if (addr % sizeof(T) != 0)
		panic("Unaligned read at address: 0x", addr);

	uint32_t abs_addr = mask_region(addr);

	if (auto offset = EXPANSION_1.contains(abs_addr); offset.has_value())
		return 0xff;
	if (auto offset = SPU.contains(abs_addr); offset.has_value()) {
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
		uint32_t off = (offset.value() & 0x0f) >> 4;
		return timers[off].read(offset.value());
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
void Bus::write(uint32_t addr, T data)
{
	if (addr % sizeof(T) != 0) {
		printf("Unaligned write to address: 0x%x\n", addr);
		exit(0);
	}

	uint32_t abs_addr = mask_region(addr);

	if (auto offset = TIMERS.contains(abs_addr); offset.has_value()) { // Ignore Timer write
		uint32_t off = (offset.value() & 0x0f) >> 4;
		return timers[off].write(offset.value(), data);
	}
	else if (auto offset = EXPANSION_2.contains(abs_addr); offset.has_value()) // Ignore expansion 2 writes
		return;
	if (auto offset = SPU.contains(abs_addr); offset.has_value()) // Ignore SPU write
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
	if (auto offset = DMA.contains(abs_addr); offset.has_value()) {
		printf("DMA write to address: 0x%x with data 0x%x\n", addr, data);
		return dma.write(offset.value(), data);
	}
	if (auto offset = RAM.contains(abs_addr); offset.has_value())
		return ram->write<T>(offset.value(), data);
	else if (auto offset = CACHE_CONTROL.contains(abs_addr); offset.has_value()) {
		cache_ctrl.raw = data;
		return;
	}
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

/* Template definitions */
template uint32_t Bus::read<uint32_t>(uint32_t);
template uint16_t Bus::read<uint16_t>(uint32_t);
template uint8_t Bus::read<uint8_t>(uint32_t);

template void Bus::write<uint32_t>(uint32_t, uint32_t);
template void Bus::write<uint16_t>(uint32_t, uint16_t);
template void Bus::write<uint8_t>(uint32_t, uint8_t);
