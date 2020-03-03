#include "psx.h"
#include <cpu/cpu.h>
#include "video/renderer.h"

PSX::PSX(Renderer* renderer)
{
	gl_renderer = renderer;
	bus = std::make_unique<Bus>("SCPH1001.BIN", gl_renderer);
	cpu = std::make_unique<CPU>(bus.get());
	gpu = std::make_unique<GPU>();

	/* Attach devices to the bus. */
	bus->cpu = cpu.get();
	bus->gpu = gpu.get();
}

void PSX::tick()
{
	for (int j = 0; j < 10; j++) {
		for (int i = 0; i < 33868800 / 60 / 10; i++) {
			/* Execute a machine cycle and handle interrupts. */
			cpu->tick();
			cpu->handle_interrupts();
			
			/* Update the timers. */
			for (int i = 0; i < 3; i++) 
				bus->timers[i].tick();
		}
		
		/* Update the CDROM drive. */
		bus->cdrom.tick();
	}

	/* At the end of the frame post VBLANK. */
	bus->irq(Interrupt::VBLANK);
	gl_renderer->update();
}