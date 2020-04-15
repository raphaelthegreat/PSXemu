#pragma once
#include <wx/wx.h>
#include <wx/glcanvas.h>

class GLViewport : public wxGLCanvas {
public:
    GLViewport(wxWindow* parent);
    ~GLViewport();
    
    void SetupGraphics();
    void OnPaint(wxPaintEvent& event);

private:
    wxGLContext* gl_context = nullptr;
};