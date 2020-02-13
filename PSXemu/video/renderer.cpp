#include "renderer.h"
#include <GLFW/glfw3.h>

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
}

Renderer::~Renderer()
{
    glfwTerminate();
}

void Renderer::push_triangle(Verts& pos, Colors& colors)
{
    glBegin(GL_TRIANGLES);
    /* Triangle */
    for (int i = 0; i < 3; i++) {
        Color& c = colors[i];
        Pos2& p = pos[i];

        glColor3ub(c.red, c.green, c.blue);
        glVertex2i(p.x + offsetx, p.y + offsety);
    }
    glEnd();
}

void Renderer::push_quad(Verts& pos, Colors& colors)
{
    glBegin(GL_TRIANGLES);
    /* Triangle 0 */
    for (int i = 0; i < 3; i++) {
        Color& c = colors[i];
        Pos2& p = pos[i];

        glColor3ub(c.red, c.green, c.blue);
        glVertex2i(p.x + offsetx, p.y + offsety);
    }
    /* Triangle 1. */
    for (int i = 1; i < 4; i++) {
        Color& c = colors[i];
        Pos2& p = pos[i];

        glColor3ub(c.red, c.green, c.blue);
        glVertex2i(p.x + offsetx, p.y + offsety);
    }
    glEnd();
}

void Renderer::push_image(TextureBuffer& buffer)
{
    double x = buffer.top_left.first;
    double y = buffer.top_left.second;

    double width = buffer.width;
    double height = buffer.height;

    printf("Image x: %.1f y: %.1f width: %.1f  height: %.1f\n", x, y, width, height);

    texture = buffer.texture();
}

void Renderer::set_draw_offset(int16_t x, int16_t y)
{
    offsetx = x;
    offsety = y;
}

void Renderer::update()
{
    glfwPollEvents();
    glfwSwapBuffers(window);

    glClear(GL_COLOR_BUFFER_BIT);
}

bool Renderer::is_open()
{
	return !glfwWindowShouldClose(window);
}

void Renderer::resize_func(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

double map(double x, double in_min, double in_max, double out_min, double out_max)
{
    return ((x - in_min) / (in_max - in_min) * (out_max - out_min) + out_min);
}