#include "stdafx.hpp"
#include "mem_widget.hpp"
#include <imgui_memory_editor.h>
#include "debugger.hpp"
#include <memory/bus.h>

void MemWidget::execute()
{
	static MemoryEditor mem_edit;
	mem_edit.DrawWindow(name.c_str(), debugger->bus->ram, 2048 * 1024);
}
