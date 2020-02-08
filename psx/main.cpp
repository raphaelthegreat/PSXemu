#include <video/renderer.h>
#include <psx.h>

int main()
{
	Renderer renderer(1024, 768, "PSX Emulator");
	PSX emulator(&renderer);
	std::vector<int> f;

	int i = std::distance(f.begin(), f.begin() + 3);
	printf("%d\n", i);

	while (renderer.is_open()) {
		emulator.tick();
		//renderer.update();
	}
}