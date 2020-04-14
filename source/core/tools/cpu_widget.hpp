#pragma once
#include "widget.hpp"

class CPU;
class CPUWidget : public Widget {
public:
	CPUWidget(CPU* _cpu) : Widget("CPU Registers"), cpu(_cpu) {}
	~CPUWidget() = default;

	void execute() override;

private:
	CPU* cpu;
};