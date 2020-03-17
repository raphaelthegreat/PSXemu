#include <video/renderer.h>
#include <psx.h>

int main()
{
	Renderer renderer(640, 512, "PSX Emulator");
	PSX emulator(&renderer);

	std::string game_file = "C:\\Users\\Alex\\Desktop\\PSXemu\\PSXemu\\Bust-a-Move2.bin";
	emulator.bus->cddrive.insert_disk_file(game_file);

	while (renderer.is_open()) {
		emulator.tick();
	}
}