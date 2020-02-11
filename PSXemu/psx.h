#pragma once
#include <cpu/cpu.h>

class Renderer;
class PSX {
public:
	PSX(Renderer* renderer);
	~PSX() = default;

	void tick();

private:
	unique_ptr<CPU> cpu;
	unique_ptr<Bus> bus;
	unique_ptr<GPU> gpu;

	Renderer* gl_renderer;

	uint32_t cycles_per_frame = 300;
};