#include <video/renderer.h>
#include <psx.h>

#pragma optimize("", off)

int main()
{
	Renderer renderer(1024, 512, "PSX Emulator");
	PSX emulator(&renderer);

	while (renderer.is_open()) {
		emulator.tick();
	}
}