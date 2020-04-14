#include "cpu_widget.hpp"
#include "imgui_header.hpp"
#include <cpu/cpu.h>

void CPUWidget::execute()
{
	ImGui::Begin(name.c_str());

	ImGui::Text("PC: 0x%x", cpu->pc);
	ImGui::Text("HI: 0x%x", cpu->hi);
	ImGui::Text("LO: 0x%x\n", cpu->lo);

	ImGui::Text("General Purpose");
	for (int i = 0; i < 32; i++) {
		ImGui::Text("%x: 0x%x", cpu->registers[i]);
	}

	ImGui::End();
}
