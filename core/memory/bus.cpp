#include <stdafx.hpp>
#include "bus.h"
#include <cpu/cpu.h>
#include <video/renderer.h>

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
	dma(this), cddrive(this)
{
    gl_renderer = renderer;
    
	/* Construct memory regions. */
    bios = std::make_unique<Bios>(bios_path);
    ram = std::make_unique<Ram>();

	controller.bus = this;
}

uint Bus::physical_addr(uint addr)
{
    uint index = addr >> 29;
    return (addr & region_mask[index]);
}

std::tuple<uint, uint, uint, uint> Bus::loadEXE(std::string test) {
	std::ifstream inFile(test, std::ios_base::binary);

	inFile.seekg(0, std::ios_base::end);
	size_t length = inFile.tellg();
	inFile.seekg(0, std::ios_base::beg);

	std::vector<ubyte> exe;
	exe.reserve(length);
	std::copy(std::istreambuf_iterator<char>(inFile),
		std::istreambuf_iterator<char>(),
		std::back_inserter(exe));
		
	uint PC = (uint)exe[0x10];
	uint R28 = (uint)exe[0x14];
	uint R29 = (uint)exe[0x30];
	uint R30 = R29; //base
	R30 += (uint)exe[0x34]; //offset
	uint DestAdress = (uint)exe[0x18];

	for (int i = 0; i < exe.size() - 0x800; i++) {
		ubyte data = exe[0x800 + i];
		ram->write<ubyte>((DestAdress & 0x1FFFFF) + i, data);
	}

	return std::make_tuple(PC, R28, R29, R30);
}

void Bus::tick()
{
	dma.tick();
	cddrive.tick();

	for (int i = 0; i < 3; i++) {
		timers[i].tick(300);
		timers[i].gpu_sync(gpu->in_hblank, gpu->in_vblank);
	}

	controller.tick();

	if (gpu->tick(300)) {
		gl_renderer->update();
		irq(Interrupt::VBLANK);
	}
}

void Bus::irq(Interrupt interrupt) {
	cpu->trigger(interrupt);
}

template<typename T>
T Bus::read(uint addr)
{
	if (addr % sizeof(T) != 0) {
		printf("Unaligned read at address: 0x%x\n", addr);
		exit(0);
	}
	
	if (addr == 0x1f801014) {
		return 0;
	}

	/* Map the memory ranges. */
	uint abs_addr = physical_addr(addr);
	if (TIMERS.contains(abs_addr)) {
		ubyte timer = (abs_addr >> 4) & 3;
		return timers[timer].read(abs_addr);
	}
	else if (RAM.contains(abs_addr)) {
		return ram->read<T>(abs_addr);
	}
	else if (BIOS.contains(abs_addr)) {
		return bios->read<T>(abs_addr);
	}
	else if (SCRATCHPAD.contains(abs_addr)) {
		return scratchpad.read<T>(abs_addr);
	}
	else if (EXPANSION_1.contains(abs_addr)) {
		return 0xff;
	}
	else if (EXPANSION_2.contains(abs_addr)) {
		T* data = (T*)ex1;
		int index = EXPANSION_2.offset(abs_addr) / sizeof(T);
		return data[index];
	}
	else if (CDROM.contains(abs_addr)) {
		if (std::is_same<T, ubyte>::value)
			return cddrive.read(abs_addr);
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
	else if (RAM_SIZE.contains(abs_addr)) {
		return 0;
	}
	else if (SPU_RANGE.contains(abs_addr)) {
		T* data = (T*)registers;
		int index = SPU_RANGE.offset(abs_addr) / sizeof(T);
		return data[index];
	}
	else if (INTERRUPT.contains(abs_addr)) {
		return cpu->read_irq(abs_addr);
	}

	int width = sizeof(T);
	printf("Unhandled read to address: 0x%x with width %d\n", abs_addr, width);
 	__debugbreak();
	return 0;
}

template<typename T>
void Bus::write(uint addr, T value)
{
	if (addr % sizeof(T) != 0) {
		printf("Unaligned write to address: 0x%x\n", addr);
		exit(0);
	}

	if (addr == 0x1f801014) {
		return;
	}

	/* Map the memory ranges. */
	uint abs_addr = physical_addr(addr);
	if (TIMERS.contains(abs_addr)) {
		ubyte timer = (abs_addr >> 4) & 3;
		return timers[timer].write(abs_addr, (uint)value);
	}
	else if (EXPANSION_2.contains(abs_addr)) {
		T* data = (T*)ex1;
		int index = EXPANSION_2.offset(abs_addr) / sizeof(T);
		data[index] = value;
		return;
	}
	else if (SPU_RANGE.contains(abs_addr)) {
		T* data = (T*)registers;
		int index = SPU_RANGE.offset(abs_addr) / sizeof(T);
		data[index] = value;
		return;
	}
	else if (GPU_RANGE.contains(abs_addr)) {
		return gpu->write(abs_addr, (uint)value);
	}
	else if (PAD_MEMCARD.contains(abs_addr)) {
		return controller.write<T>(abs_addr, value);
	}
	else if (SCRATCHPAD.contains(abs_addr)) {
		return scratchpad.write<T>(abs_addr, value);
	}
	else if (CDROM.contains(abs_addr)) {
		if (std::is_same<T, ubyte>::value)
			return cddrive.write(abs_addr, (ubyte)value);
	}
	else if (DMA_RANGE.contains(abs_addr)) {
		return dma.write(abs_addr, (uint)value);
	}
	else if (RAM.contains(abs_addr)) {
		return ram->write<T>(abs_addr, value);
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
		return cpu->write_irq(abs_addr, value);
	}

	int width = sizeof(T);
	printf("Unhandled write to address: 0x%x with width %d\n", abs_addr, width);
	exit(0);
}

/* Template definitions. */
template uint Bus::read<uint>(uint);
template ushort Bus::read<ushort>(uint);
template ubyte Bus::read<ubyte>(uint);

template void Bus::write<uint>(uint, uint);
template void Bus::write<ushort>(uint, ushort);
template void Bus::write<ubyte>(uint, ubyte);