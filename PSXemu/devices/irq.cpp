#include "irq.h"
#include <cpu/cpu.h>
#include <cstdio>
#include <cstdlib>

void IRQManager::clear()
{
	/* Clear registers. */
	irq_mask = 0;
	irq_status = 0;
}

void IRQManager::irq_request(Interrupt irq)
{
	/* Set correspoding bit in the I_STAT register. */
	set_bit(irq_status, (uint32_t)irq, true);
	
	/* Update extrnal IRQ in the CPU. */
	bool pending = (irq_mask & irq_status) != 0;
	cpu->set_ext_irq(pending);
}

bool IRQManager::irq_pending()
{
	Cop0CAUSE& cause = cpu->cop0.cause;
	Cop0STAT& sr = cpu->cop0.sr;

	/* Mask bits [8:10] in cause with the ones from sr. */
	bool pending = (cause.raw & sr.raw) & 0x700;
	
	/* Mask the result with the global irq enable flag. */
	return sr.IEc & pending;
}

uint32_t IRQManager::read(uint32_t offset)
{
	printf("IRQ read at offset: 0x%x\n", offset);
	
	switch (offset) {
	case 0: /* IRQ Status. */
		return irq_status;
	case 4: /* IRQ Mask */
		return irq_mask;
	default:
		printf("Unhandled read to IRQ control at offset: 0x%x\n", offset);
		exit(0);
	}
}

void IRQManager::write(uint32_t offset, uint32_t data)
{
	printf("IRQ write at offset: 0x%x with data: 0x%x\n", offset, data);

	switch (offset) {
	case 0: /* IRQ Status. */
		irq_status &= data & 0x3FF;
		break;
	case 4: /* IRQ Mask. */
		irq_mask = data & 0x3FF;
		break;
	default:
		printf("Unhandled write to IRQ control at offset: 0x%x with data: 0x%x\n", offset, data);
		exit(0);
	}

	/* Update extrnal IRQ in the CPU. */
	bool pending = (irq_mask & irq_status) != 0;
	cpu->set_ext_irq(pending);
}
