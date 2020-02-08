#include "psx.h"
#include <video/renderer.h>

PSX::PSX(Renderer* renderer)
{
	gl_renderer = renderer;
	psx_cpu = std::make_unique<CPU>(gl_renderer);
}

void PSX::tick()
{
	psx_cpu->tick();
}
