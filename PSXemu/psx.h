#pragma once
#include <cpu/cpu.h>

class Renderer;
class PSX {
public:
	PSX(Renderer* renderer);
	~PSX() = default;

	void tick();

private:
	unique_ptr<CPU> psx_cpu;
	Renderer* gl_renderer;
};