#include <video/renderer.h>
#include <psx.h>

//#pragma optimize("", off)

int main()
{
	Renderer renderer(1024, 512, "PSX Emulator");
	PSX emulator(&renderer);

	std::string game_file = "C:\\Users\\Alex\\Desktop\\PSXemu\\PSXemu\\Crash_Bandicoot.bin";
	emulator.bus->cddrive.insert_disk_file(game_file);

	while (renderer.is_open()) {
		emulator.tick();
	}
}