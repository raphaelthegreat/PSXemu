#include <video/renderer.h>
#include <psx.h>

int main()
{
	Renderer renderer(1024, 768, "PSX Emulator");
	PSX emulator(&renderer);

	while (renderer.is_open()) {
		emulator.tick();
		//renderer.update();
	}
}