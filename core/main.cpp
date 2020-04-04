#include <stdafx.hpp>
#include <video/renderer.h>
#include <psx.h>

int main()
{
	Renderer renderer(640, 480, "PSX Emulator");
	PSX emulator(&renderer);

	std::string game_file = "C:\\Users\\Alex\\Desktop\\PSXemu-texturing\\roms\\Crash_Bandicoot.bin";
	emulator.bus->cddrive.insert_disk(game_file);

	while (renderer.is_open()) {
		emulator.tick();
	}
}