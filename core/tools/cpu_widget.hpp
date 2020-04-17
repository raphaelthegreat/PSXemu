#pragma once
#include "widget.hpp"

class Debugger;
class CPUWidget : public Widget {
public:
    CPUWidget(Debugger* _debugger) :
        Widget("CPU Registers"), debugger(_debugger) {}
	~CPUWidget() = default;

	void execute() override;

private:
	Debugger* debugger;
    const std::string reg_table[32] =
    {
        "ZR", "AT", "V0", "V1", "A0", "A1", "A2", "A3", "T0",
        "T1", "T2", "T3", "T4", "T5", "T6", "T7", "S0", "S1",
        "S2", "S3", "S4", "S5", "S6", "S7", "T8", "T9", "K0",
        "K1", "GP", "SP", "FP", "RA"
    };
};