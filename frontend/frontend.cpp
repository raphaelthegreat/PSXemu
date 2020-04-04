#include "frontend.hpp"

wxIMPLEMENT_APP(Frontend);

Frontend::Frontend()
{
}

Frontend::~Frontend()
{
}

bool Frontend::OnInit()
{
	window_frame = new Window();
	window_frame->Show();

	return true;
}
