#include "psx.h"
#include <cpu/cpu.h>
#include "video/renderer.h"

PSX::PSX(Renderer* renderer)
{
	gl_renderer = renderer;
	bus = std::make_unique<Bus>("SCPH1001.BIN", gl_renderer);
	cpu = std::make_unique<CPU>(bus.get());

	/* Attach devices to the bus. */
	bus->cpu = cpu.get();
}

void PSX::tick()
{
	for (int j = 0; j < 10; j++) {
		for (int i = 0; i < 33868800 / 60 / 10; i++) {
			cpu->tick();
			cpu->handle_interrupts();
			bus->timers[2].tick();
		}
		
		bus->cdrom.tick();
	}

	bus->irq(Interrupt::VBLANK);
	gl_renderer->update();
}

bool PSX::render()
{
	return gl_renderer->is_open();
}
