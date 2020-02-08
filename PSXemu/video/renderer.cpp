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

    shader = std::make_unique<Shader>();
    
    shader->load("shaders/vertex.glsl", ShaderType::Vertex);
    shader->load("shaders/fragment.glsl", ShaderType::Fragment);
    shader->build();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    vertex_buffer = std::make_unique<Buffer<Pos2>>();
    vertex_buffer->bind();

    glVertexAttribIPointer(0, 2, GL_SHORT, sizeof(Pos2), (void*)0);
    glEnableVertexAttribArray(0);

    color_buffer = std::make_unique<Buffer<Color>>();
    color_buffer->bind();

    glVertexAttribIPointer(1, 3, GL_UNSIGNED_BYTE, sizeof(Color), (void*)0);
    glEnableVertexAttribArray(1);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &vao);
    glfwTerminate();
}

void Renderer::push_triangle(Verts& pos, Colors& colors)
{
    if (count + 3 > BUFFER_SIZE) // Force draw if pending overflow
        draw_scene();

    for (int i = 0; i < 3; i++) {
        vertex_buffer->set(count, pos[i]);
        color_buffer->set(count, colors[i]);
        count++;
    }
}

void Renderer::push_quad(Verts& pos, Colors& colors)
{
    if (count + 6 > BUFFER_SIZE)
        draw_scene();

    for (int i = 0; i < 3; i++) {
        vertex_buffer->set(count, pos[i]);
        color_buffer->set(count, colors[i]);
        count++;
    }

    for (int i = 1; i < 4; i++) {
        vertex_buffer->set(count, pos[i]);
        color_buffer->set(count, colors[i]);
        count++;
    }
}

void Renderer::draw_scene()
{
    glClear(GL_COLOR_BUFFER_BIT);
    shader->bind();
    
    glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, count);

    GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    while (true) {
        auto status = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 10000000);
        if (status == GL_ALREADY_SIGNALED || 
            status == GL_CONDITION_SATISFIED)
            break;
    }
    
    count = 0;
}

void Renderer::set_draw_offset(int16_t x, int16_t y)
{
    this->draw_scene();
    
    auto loc = glGetUniformLocation(shader->raw(), "offset");
    glUniform2i(loc, x, y);
}

void Renderer::update()
{
	glfwPollEvents();
	glfwSwapBuffers(window);
}

bool Renderer::is_open()
{
	return !glfwWindowShouldClose(window);
}

void Renderer::resize_func(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}
