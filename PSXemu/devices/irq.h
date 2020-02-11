#pragma once
#include <cstdint>

enum class Interrupt {
	VBlank = 0x0,
	GPU = 0x1,
	CDROM = 0x2,
	DMA = 0x3,
	TMR0 = 0x4,
	TMR1 = 0x5,
	TMR2 = 0x6,
	IRQ7 = 0x7,
	SIO = 0x8,
	SPU = 0x9,
	IRQ10 = 0xA
};

class CPU;
class IRQManager {
public:
	IRQManager() = default;
	~IRQManager() = default;

	/* Clears all pending interrputs. */
	void clear();
	/* Request an interrupt. */
	void irq_request(Interrupt irq);
	/* Check if any interuupt is pending. */
	bool irq_pending();

	uint32_t read(uint32_t offset);
	void write(uint32_t offset, uint32_t data);

public:
	uint32_t irq_status = 0;
	uint32_t irq_mask = 0;

	CPU* cpu;
};