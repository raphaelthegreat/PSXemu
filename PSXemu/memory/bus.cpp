#include "bus.h"
#include <cpu/cpu.h>
#include <video/renderer.h>
#pragma optimize("", off)

const Range SYS_CONTROL = Range(0x1f801000, 36);
const Range RAM_SIZE = Range(0x1f801060, 4);
const Range CACHE_CONTROL = Range(0xfffe0130, 4);
const Range SPU_RANGE = Range(0x1f801c00, 640);
const Range EXPANSION_2 = Range(0x1f802000, 66);
const Range EXPANSION_1 = Range(0x1f000000, 512 * 1024);
const Range TIMERS = Range(0x1f801100, 0x30);
const Range PAD_MEMCARD = Range(0x1f801040, 15);
const Range CDROM = Range(0x1f801800, 0x4);

Bus::Bus(std::string bios_path, Renderer* renderer) :
	dma(this)
{
    gl_renderer = renderer;
    
	/* Construct memory regions. */
    bios = std::make_unique<Bios>(bios_path);
    ram = std::make_unique<Ram>();

	controller.bus = this;
	cddrive.init(this);
}

uint32_t Bus::physical_addr(uint32_t addr)
{
    uint32_t index = addr >> 29;
    return (addr & region_mask[index]);
}

void Bus::tick()
{
	if (gpu->tick(300)) {
		gl_renderer->update();
		cpu->trigger(Interrupt::VBLANK);
	}

	for (int i = 0; i < 3; i++) {
		timers[i].tick(300);
		timers[i].gpu_sync(gpu->in_hblank, gpu->in_vblank);
	}

	controller.tick();
	cddrive.step();
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
	if (TIMERS.contains(abs_addr)) {
		uint8_t timer = (abs_addr >> 4) & 3;
		return timers[timer].read(abs_addr);
	}
	else if (RAM.contains(abs_addr)) {
		return ram->read<T>(abs_addr);
	}
	else if (BIOS.contains(abs_addr)) {
		return bios->read<T>(abs_addr);
	}
	else if (EXPANSION_1.contains(abs_addr)) {
		return 0xff;
	}
	else if (EXPANSION_2.contains(abs_addr)) {
		return 0;
	}
	else if (CDROM.contains(abs_addr)) {
		if (std::is_same<T, uint8_t>::value)
			return cddrive.read_reg(abs_addr);
	}
	else if (GPU_RANGE.contains(abs_addr)) {
		return gpu->read(abs_addr);
	}
	else if (PAD_MEMCARD.contains(abs_addr)) {
		return controller.read<T>(abs_addr);
	}
	else if (DMA_RANGE.contains(abs_addr)) {
		return dma.read(abs_addr);
	}
	else if (SPU_RANGE.contains(abs_addr)) {
		return 0;
	}
	else if (INTERRUPT.contains(abs_addr)) {
		return cpu->read_irq(abs_addr);
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
		return timers[timer].write(abs_addr, data);
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
		return controller.write<T>(abs_addr, data);
	}
	else if (CDROM.contains(abs_addr)) {
		if (std::is_same<T, uint8_t>::value)
			return cddrive.write_reg(abs_addr, data);
	}
	else if (DMA_RANGE.contains(abs_addr)) {
		return dma.write(abs_addr, data);
	}
	else if (RAM.contains(abs_addr)) {
		return ram->write<T>(abs_addr, data);
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
		return cpu->write_irq(abs_addr, data);
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