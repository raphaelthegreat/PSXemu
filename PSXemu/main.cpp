#include <video/renderer.h>
#include <psx.h>

#pragma optimize("", off)

int main()
{
	Renderer renderer(1024, 768, "PSX Emulator");
	PSX emulator(&renderer);

	while (emulator.render()) {
		int j = 0;
		emulator.tick();
	}
}