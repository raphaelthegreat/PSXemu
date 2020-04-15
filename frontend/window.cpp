#include "window.hpp"

Window::Window() :
	wxFrame(nullptr, wxID_ANY, "Playstation 1 Emulator", 
			wxPoint(30, 30), wxSize(800, 600))
{
	button = new wxButton(this, wxID_ANY, "Open");
	canvas = new GLViewport(this);
}

Window::~Window()
{
	delete canvas;
	delete button;
}
