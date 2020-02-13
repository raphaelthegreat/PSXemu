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
	cpu->tick();

	/* Tick the GPU. */
	gpu->tick(cycles_per_frame);

	/* Trigger Vblank interrupt. */
	/*if (gpu->is_vblank()) {
		bus->irq_manager.irq_request(Interrupt::VBlank);
		return;
	}

	if (gpu->scanline > VBLANK_START) {
		Timer& t = bus->timers[1];
		SyncMode sync = t.get_sync_mode();

		if (t.mode.sync_enable) {
			
			if (sync == SyncMode::Reset || sync == SyncMode::ResetPause) {
				t.current.raw = 0;
			}
			else if (sync == SyncMode::PauseFreeRun) {
			    t.paused = false;
				t.mode.sync_enable = false;
			}
		}
	}*/
}
