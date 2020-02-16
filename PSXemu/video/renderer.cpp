#include "renderer.h"
#include <GLFW/glfw3.h>

void CHECK_FRAMEBUFFER_STATUS()
{
    GLenum status;
    status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    switch (status) {
    case GL_FRAMEBUFFER_COMPLETE:
        break;

    case GL_FRAMEBUFFER_UNSUPPORTED:
        /* choose different formats */
        break;

    default:
        /* programming error; will fail on all hardware */
        fputs("Framebuffer Error\n", stderr);
        exit(-1);
    }
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
        Color8& c = colors[i];
        Pos2i& p = pos[i];

        glColor3ub(c.red, c.green, c.blue);
        glVertex2i(p.x + offsetx, p.y + offsety);
    }
    /* Triangle 1. */
    for (int i = 1; i < 4; i++) {
        Color8& c = colors[i];
        Pos2i& p = pos[i];

        glColor3ub(c.red, c.green, c.blue);
        glVertex2i(p.x + offsetx, p.y + offsety);
    }
    glEnd();
}

void Renderer::push_textured_quad(Verts& pos, Coords& coords, TextureInfo& info)
{
    Texture8* texture = textures[info];

    if (texture != NULL)
        texture->bind();

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);

    /* Triangle 0. */
    for (int i = 0; i < 3; i++) {
        glTexCoord2f(coords[i].x, coords[i].y);
        glVertex2i(pos[i].x, pos[i].y);
    }

    /* Triangle 1. */
    for (int i = 1; i < 4; i++) {
        glTexCoord2f(coords[i].x, coords[i].y);
        glVertex2i(pos[i].x, pos[i].y);
    }

    glEnd();
    
    if (texture != NULL)
        texture->unbind();

    glDisable(GL_TEXTURE_2D);
}

void Renderer::update_texture(TextureInfo& info, Pixels& pixels)
{
    glEnable(GL_TEXTURE_2D);
    
    /* Update texture if it exists else create it. */
    Texture8* texture = textures[info];
    if (texture != NULL) {
        texture->bind();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
                                        info.width, 
                                        info.height, 
                                        (int)info.format, 
                                        GL_UNSIGNED_BYTE, 
                                        &pixels.front());

        texture->unbind();
    }
    else {
        textures[info] = new Texture8(info.width, info.height, &pixels.front(), Format::RGBA, Filtering::Nearest);
    }

    glDisable(GL_TEXTURE_2D);
}

bool Renderer::is_texture_null(TextureInfo& info)
{
    return (textures[info] == NULL);
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