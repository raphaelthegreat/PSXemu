#include "psx.h"

PSX::PSX(Renderer* renderer)
{
	gl_renderer = renderer;
	bus = std::make_unique<Bus>("SCPH1001.BIN", gl_renderer);
	cpu = std::make_unique<CPU>(bus.get());
	gpu = std::make_unique<GPU>(gl_renderer);

	/* Attach devices to the bus. */
	bus->cpu = cpu.get();
	bus->gpu = gpu.get();
}

void PSX::tick()
{
	/* execute a frame's worth of cpu cycles. */
	cpu->tick();

	/* Tick the GPU. */
	gpu->tick(cycles_per_frame);
}
