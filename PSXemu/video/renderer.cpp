#include "renderer.h"
#include <GLFW/glfw3.h>
#include <cpu/util.h>
#include <video/vram.h>

Renderer::Renderer(int _width, int _height, std::string title)
{
	glfwInit();
	window = glfwCreateWindow(_width, _height, title.c_str(), NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Could not load OpenGL!\n");
        __debugbreak();
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	width = _width; height = _height;
	glfwSetFramebufferSizeCallback(window, resize_func);

    /* Build shader */
    shader = std::make_unique<Shader>();
    shader->load("shaders/vertex.vert", ShaderType::Vertex);
    shader->load("shaders/fragment.frag", ShaderType::Fragment);
    shader->build();
    
    /* Create the screen texture. */
    screen_texture = std::make_unique<Texture8>(1024, 512, 
                                                nullptr, 
                                                Filtering::Linear,
                                                Format::RGB);

    /* Define screen quad vertices. */
    float vertices[] = {
          1.0f, -1.0f, 1.0f, 1.0f,
         -1.0f, -1.0f, 0.0f, 1.0f,
         -1.0f,  1.0f, 0.0f, 0.0f,

          1.0f,  1.0f, 1.0f, 0.0f,
          1.0f, -1.0f, 1.0f, 1.0f,
         -1.0f,  1.0f, 0.0f, 0.0f
    };

    /* Upload them to the GPU. */
    glGenVertexArrays(1, &screen_vao);
    glBindVertexArray(screen_vao);

    glGenBuffers(1, &screen_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, screen_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (void*)8);
    glEnableVertexAttribArray(1);
}

Renderer::~Renderer()
{
    glfwTerminate();
}

void Renderer::set_draw_offset(int16_t x, int16_t y)
{
    offsetx = x;
    offsety = y;
}

void Renderer::draw_scene()
{
    /* Generate a pixel stream. */
    for (int y = 0; y < 512; y++) {
        for (int x = 0; x < 1024; x++) {
            uint32_t color = vram.read(x, y);
            
            pixels.push_back(((color << 3) & 0xf8));
            pixels.push_back(((color >> 2) & 0xf8));
            pixels.push_back(((color >> 7) & 0xf8));
        }
    }
    
    /* Update the display texture */
    screen_texture->update(&pixels.front());
    
    /* Render draw data. */
    shader->bind();
    screen_texture->bind();
    glBindVertexArray(screen_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    pixels.clear();
}

void Renderer::update()
{
    glfwPollEvents();
    draw_scene();
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