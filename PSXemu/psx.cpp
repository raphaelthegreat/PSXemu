#include "psx.h"

PSX::PSX(Renderer* renderer)
{
	gl_renderer = renderer;
	bus = std::make_unique<Bus>("SCPH1001.BIN", gl_renderer);
	cpu = std::make_unique<CPU>(gl_renderer, bus.get());
	gpu = std::make_unique<GPU>(gl_renderer);

	/* Attach devices to the bus. */
	bus->set_cpu(cpu.get());
	bus->set_gpu(gpu.get());
}

void PSX::tick()
{
	/* Execute a frame's worth of cpu cycles. */
	for (int i = 0; i < cycles_per_frame / 3; i++)
		cpu->tick();

	/* Update the timers. */
	for (auto& timer : bus->timers) {
		timer.tick(cycles_per_frame);
	}

	/* Tick the GPU. */
	gpu->tick(cycles_per_frame);

	/* Trigger Vblank interrupt. */
	if (gpu->is_vblank())
		bus->irq_manager.irq_request(Interrupt::VBlank);
}
