#include <stdafx.hpp>
#include <video/renderer.h>
#include <emulator.hpp>

int main()
{
	/* Hack */
	_wchdir(L"C://Users//Alex//Desktop//psxemu");

	Renderer renderer(1024, 512, "PSX Emulator");
	PSX emulator(&renderer);

	std::string game_file = "C:\\Users\\Alex\\Desktop\\PSXemu-texturing\\roms\\Crash_Bandicoot.bin";
	emulator.bus->cddrive.insert_disk(game_file);

	while (renderer.is_open()) {
		emulator.tick();
	}
}