#include <stdafx.hpp>
#include <video/renderer.h>
#include <memory/bus.h>

int main()
{
	auto emulator = std::make_unique<Bus>("./bios/SCPH1001.BIN");

	std::string game_file = "C:\\Users\\Alex\\Desktop\\PSXemu\\roms\\RIDGERACERUSA.BIN";
	emulator->cddrive->insert_disk(game_file);

	while (emulator->renderer->is_open()) {
		emulator->tick();
	}
}