#pragma once
#include <wx/wx.h>
#include <video/glcanvas.hpp>

class Window : public wxFrame {
public:
	Window();
	~Window();

private:
	wxButton* button = nullptr;
	GLViewport* canvas = nullptr;
};