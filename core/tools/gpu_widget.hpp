#pragma once
#include "widget.hpp"
#include <string>

struct RowDesc {
    std::string bits;
    std::string description;
    uint* value;
};

class Debugger;
class GPUWidget : public Widget {
public:
    GPUWidget(Debugger* _debugger);
	~GPUWidget() = default;

	void execute() override;

private:
	Debugger* debugger;

    std::vector<RowDesc> rows;

   /* RowDesc rows_gp0[12] =
    {
        { "GP0(E1h).12", "Textured Rectangle X-Flip", "0" },
        { "GP0(E1h).13", "Textured Rectangle Y-Flip", "0" },
        { "GP0(E2h).0-4", "Texture window Mask X", "0" },
        { "GP0(E2h).5-9", "Texture window Mask Y", "0" },
        { "GP0(E2h).10-14", "Texture window Offset X", "0" },
        { "GP0(E2h).15-19", "Texture window Offset Y", "0" },
        { "GP0(E3h).0-9", "Drawing Area left", "0" },
        { "GP0(E3h).10-18", "Drawing Area top", "0" },
        { "GP0(E4h).0-9", "Drawing Area right", "0" },
        { "GP0(E4h).10-18", "Drawing Area top bottom", "0" },
        { "GP0(E5h).0-10", "Drawing X-offset", "0" },
        { "GP0(E5h).11-21", "Drawing Y-offset", "0" },
    };*/
};