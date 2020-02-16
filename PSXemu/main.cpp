#include <video/renderer.h>
#include <psx.h>

int main()
{
	Renderer renderer(1280, 960, "PSX Emulator");
	PSX emulator(&renderer);

	while (renderer.is_open()) {
		emulator.tick();
	}
}