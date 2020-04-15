#include <glad/glad.h>
#include "glcanvas.hpp"

GLViewport::GLViewport(wxWindow* parent) :
	wxGLCanvas(parent, wxID_ANY, 0, wxDefaultPosition, wxSize(1, 2))
{
	gl_context = new wxGLContext(this);
	Bind(wxEVT_PAINT, &GLViewport::OnPaint, this);

	SetCurrent(*gl_context);
	SetupGraphics();
}

GLViewport::~GLViewport()
{
	delete gl_context;
}

void GLViewport::SetupGraphics()
{
	gladLoadGL();
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
}

void GLViewport::OnPaint(wxPaintEvent& event)
{
	glClear(GL_COLOR_BUFFER_BIT);
	SwapBuffers();
}
