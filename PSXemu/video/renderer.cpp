#include "renderer.h"
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GLU.h>
#endif

double map(double x, double in_min, double in_max, double out_min, double out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

Renderer::Renderer(int _width, int _height, std::string title)
{
	glfwInit();
	window = glfwCreateWindow(_width, _height, title.c_str(), NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		panic("Could not load OpenGL!", "");

	width = _width; height = _height;
	glfwSetFramebufferSizeCallback(window, resize_func);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    glGenFramebuffersEXT(1, &fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &color_buffer);
    glBindTexture(GL_TEXTURE_2D, color_buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 768, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_buffer, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("[OpenGL] Could not create screen surface!\n");
        exit(0);
    }
}

Renderer::~Renderer()
{
    glfwTerminate();
}

void Renderer::push_triangle(Verts& pos, Colors& colors)
{
    glBegin(GL_TRIANGLES);
    /* Triangle 0. */
    for (int i = 0; i < 3; i++) {
        Color8& c = colors[i];
        Pos2i& p = pos[i];

        double xvert = map(p.x, 0, 640, -1, 1);
        double yvert = map(p.y, 0, 480, 1, -1);

        glColor3ub(c.red, c.green, c.blue);
        glVertex2f(xvert, yvert);
    }
    glEnd();
}

void Renderer::push_quad(Verts& pos, Colors& colors)
{
    glBegin(GL_TRIANGLES);
    /* Triangle 0 */
    for (int i = 0; i < 3; i++) {
        Color8& c = colors[i];
        Pos2i& p = pos[i];

        double xvert = map(p.x, 0, 640, -1, 1);
        double yvert = map(p.y, 0, 480, 1, -1);

        glColor3ub(c.red, c.green, c.blue);
        glVertex2f(xvert, yvert);
    }
    /* Triangle 1. */
    for (int i = 1; i < 4; i++) {
        Color8& c = colors[i];
        Pos2i& p = pos[i];

        double xvert = map(p.x, 0, 640, -1, 1);
        double yvert = map(p.y, 0, 480, 1, -1);

        glColor3ub(c.red, c.green, c.blue);
        glVertex2f(xvert, yvert);
    }
    glEnd();
}

void Renderer::push_textured_quad(Verts& pos, Coords& coords, Texture8* texture)
{
    if (texture != NULL)
        texture->bind();

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);

    /* Triangle 0. */
    for (int i = 0; i < 3; i++) {
        Pos2f& c = coords[i];
        Pos2i& p = pos[i];

        double xvert = map(p.x, 0, 640, -1, 1);
        double yvert = map(p.y, 0, 480, 1, -1);
        
        glTexCoord2f(c.x, c.y);
        glVertex2f(xvert, yvert);
    }

    /* Triangle 1. */
    for (int i = 1; i < 4; i++) {
        Pos2f& c = coords[i];
        Pos2i& p = pos[i];

        double xvert = map(p.x, 0, 640, -1, 1);
        double yvert = map(p.y, 0, 480, 1, -1);
        
        glTexCoord2f(c.x, c.y);
        glVertex2f(xvert, yvert);
    }

    glEnd();
    
    if (texture != NULL)
        texture->unbind();

    glDisable(GL_TEXTURE_2D);
}

void Renderer::set_draw_offset(int16_t x, int16_t y)
{
    offsetx = x;
    offsety = y;
}

void Renderer::draw_scene()
{
    glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, color_buffer);

    glBegin(GL_TRIANGLES);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0, 1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);

    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0, 1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);

    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glBindFramebufferEXT(GL_FRAMEBUFFER, fbo);

    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::update()
{
    glfwPollEvents();
    draw_scene();
    glfwSwapBuffers(window);
}

bool Renderer::is_open()
{
	return !glfwWindowShouldClose(window);
}

void Renderer::resize_func(GLFWwindow* window, int width, int height)
{
	//glViewport(0, 0, width, height);
}