#include "bus.h"
#include <cpu/cpu.h>
#include <video/renderer.h>

const Range RAM = Range(0x00000000, 2048 * 1024);
const Range BIOS = Range(0x1fc00000, 512 * 1024);
const Range SYS_CONTROL = Range(0x1f801000, 36);
const Range CDROM = Range(0x1f801800, 0x4);
const Range RAM_SIZE = Range(0x1f801060, 4);
const Range CACHE_CONTROL = Range(0xfffe0130, 4);
const Range SPU_RANGE = Range(0x1f801c00, 640);
const Range EXPANSION_2 = Range(0x1f802000, 66);
const Range EXPANSION_1 = Range(0x1f000000, 512 * 1024);
const Range TIMERS = Range(0x1f801100, 0x30);
const Range PAD_MEMCARD = Range(0x1f801040, 15);
const Range DMA_RANGE = Range(0x1f801080, 0x80);
const Range GPU_RANGE = Range(0x1f801810, 8);
const Range INTERRUPT = Range(0x1f801070, 8);

inline bool Range::contains(uint32_t addr) const
{
	return (addr >= start && addr < start + length);
}

inline uint32_t Range::offset(uint32_t addr) const
{
	return addr - start;
}

Bus::Bus(std::string bios_path, Renderer* renderer) :
	dma(this), cdrom(this)
{
    gl_renderer = renderer;
    
	/* Construct memory regions. */
    bios = std::make_unique<Bios>(bios_path);
    ram = std::make_unique<Ram>();

	/* Init the timers. */
	for (TimerID i = TimerID::TMR0; (int)i < 3; i = (TimerID)((int)i + 1)) {
		timers[(int)i].init(i, this);
	}
}

uint32_t Bus::physical_addr(uint32_t addr)
{
    uint32_t index = addr >> 29;
    return (addr & region_mask[index]);
}

void Bus::tick()
{
	cdrom.tick();
	dma.tick();
}

void Bus::irq(Interrupt interrupt) {
	cpu->trigger(interrupt);
}

template<typename T>
T Bus::read(uint32_t addr)
{
	if (addr % sizeof(T) != 0) {
		printf("Unaligned read at address: 0x%x\n", addr);
		exit(0);
	}

	/* Map the memory ranges. */
	uint32_t abs_addr = physical_addr(addr);
	if (RAM.contains(abs_addr)) {
		uint32_t offset = RAM.offset(abs_addr);
		return ram->read<T>(offset);
	}
	else if (BIOS.contains(abs_addr)) {
		uint32_t offset = BIOS.offset(abs_addr);
		return bios->read<T>(offset);
	}
	else if (EXPANSION_1.contains(abs_addr)) {
		return 0xff;
	}
	else if (EXPANSION_2.contains(abs_addr)) {
		return 0;
	}
	else if (CDROM.contains(abs_addr)) {
		uint32_t offset = CDROM.offset(abs_addr);
		return cdrom.read(offset);
	}
	else if (GPU_RANGE.contains(abs_addr)) {
		uint32_t offset = GPU_RANGE.offset(abs_addr);
		return gpu->read(offset);
	}
	else if (PAD_MEMCARD.contains(abs_addr)) {
		return 0xffffffff;
	}
	else if (TIMERS.contains(abs_addr)) {
		uint8_t timer = (abs_addr >> 4) & 3;
		uint8_t offset = abs_addr & 0xf;
		return timers[timer].read(offset);
	}
	else if (DMA_RANGE.contains(abs_addr)) {
		uint32_t offset = DMA_RANGE.offset(abs_addr);
		return dma.read(offset);
	}
	else if (SPU_RANGE.contains(abs_addr)) {
		return 0;
	}
	else if (INTERRUPT.contains(abs_addr)) {
		uint32_t offset = INTERRUPT.offset(abs_addr);
		return cpu->read_irq(offset);
	}

	printf("Unhandled read to address: 0x%x\n", addr);
	exit(0);
	return 0;
}

template<typename T>
void Bus::write(uint32_t addr, T data)
{
	if (addr % sizeof(T) != 0) {
		printf("Unaligned write to address: 0x%x\n", addr);
		exit(0);
	}

	/* Map the memory ranges. */
	uint32_t abs_addr = physical_addr(addr);
	if (TIMERS.contains(abs_addr)) {
		uint8_t timer = (abs_addr >> 4) & 3;
		uint8_t offset = abs_addr & 0xf;
		return timers[timer].write(offset, data);
	}
	else if (EXPANSION_2.contains(abs_addr)) {
		return;
	}
	else if (SPU_RANGE.contains(abs_addr)) {
		return;
	}
	else if (GPU_RANGE.contains(abs_addr)) {
		return gpu->write(abs_addr, data);
	}
	else if (PAD_MEMCARD.contains(abs_addr)) {
		return;
	}
	else if (CDROM.contains(abs_addr)) {
		uint32_t offset = CDROM.offset(abs_addr);
		return cdrom.write(offset, data);
	}
	else if (DMA_RANGE.contains(abs_addr)) {
		uint32_t offset = DMA_RANGE.offset(abs_addr);
		return dma.write(offset, data);
	}
	else if (RAM.contains(abs_addr)) {
		uint32_t offset = RAM.offset(abs_addr);
		return ram->write<T>(offset, data);
	}
	else if (CACHE_CONTROL.contains(abs_addr)) {
		return;
	}
	else if (RAM_SIZE.contains(abs_addr)) {
		return;
	}
	else if (SYS_CONTROL.contains(abs_addr)) {
		return;
	}
	else if (INTERRUPT.contains(abs_addr)) {
		uint32_t offset = INTERRUPT.offset(abs_addr);
		return cpu->write_irq(offset, data);
	}

	printf("Unhandled write to address: 0x%x\n", addr);
	exit(0);
}

/* Template definitions. */
template uint32_t Bus::read<uint32_t>(uint32_t);
template uint16_t Bus::read<uint16_t>(uint32_t);
template uint8_t Bus::read<uint8_t>(uint32_t);

template void Bus::write<uint32_t>(uint32_t, uint32_t);
template void Bus::write<uint16_t>(uint32_t, uint16_t);
template void Bus::write<uint8_t>(uint32_t, uint8_t);