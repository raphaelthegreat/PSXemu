#include "psx.h"
#include "video/renderer.h"

PSX::PSX(Renderer* renderer)
{
	gl_renderer = renderer;
	bus = std::make_unique<Bus>("SCPH1001.BIN", gl_renderer);
	cpu = std::make_unique<CPU>(bus.get());
	gpu = std::make_unique<GPU>(gl_renderer);

	/* Attach devices to the bus. */
	bus->cpu = cpu.get();
	bus->gpu = gpu.get();

	gl_renderer->set_vram(&gpu->vram);
}

void PSX::tick()
{
	for (int j = 0; j < 10; j++) {
		for (int i = 0; i < 33868800 / 60 / 10; i++) {
			cpu->tick();
			bus->timers[2].tick();
		}

		bus->tick();
	}

	bus->interruptController.set(Interrupt::GPU_IRQ);
}

bool PSX::render()
{
	gl_renderer->update();
	
	return gl_renderer->is_open();
}
