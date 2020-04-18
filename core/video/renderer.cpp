#include <stdafx.hpp>
#include <glad/glad.h>
#include "renderer.h"
#include <memory/bus.h>
#include <video/vram.h>

Renderer::Renderer(int width, int height, const std::string& title, Bus* _bus) :
    bus(_bus)
{
    window_width = width;
    window_height = height;

    glfwInit();
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("[OPENGL] gladLoadGLLoader: failed to load OpenGL.\n");
        exit(1);
    }

    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    /* Build shader */
    shader = std::make_unique<Shader>();
    shader->load("data/shaders/vertex.vert", ShaderType::Vertex);
    shader->load("data/shaders/fragment.frag", ShaderType::Fragment);
    shader->build();

    /* Create the screen framebuffer. */
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &framebuffer_texture);
    glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer_texture, 0);

    glViewport(0, 0, VRAM_WIDTH, VRAM_HEIGHT);
    glScissor(0, 0, VRAM_WIDTH, VRAM_HEIGHT);

    glGenRenderbuffers(1, &framebuffer_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, framebuffer_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1024, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebuffer_rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("[OPENGL] glCheckFramebufferStatus: failed not create screen framebuffer.\n");
        exit(1);
    }

    /* Build vertex buffer. */
    glGenVertexArrays(1, &draw_vao);
    glBindVertexArray(draw_vao);

    glGenBuffers(1, &draw_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, draw_vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * MAX_VERTICES, nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, coord));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texpage));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, clut_coord));
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, textured));
    glEnableVertexAttribArray(5);

    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color_depth));
    glEnableVertexAttribArray(6);

    draw_data.reserve(MAX_VERTICES);
    vram.init();
}

Renderer::~Renderer()
{
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &framebuffer_texture);

    glDeleteBuffers(1, &draw_vbo);
    glDeleteVertexArrays(1, &draw_vao);

    glfwTerminate();
}

void Renderer::draw_call(std::vector<Vertex>& data, Primitive p)
{
    draw_data.insert(draw_data.end(), data.begin(), data.end());
}

void Renderer::draw(std::vector<Vertex>& data)
{
    /* Get current display resolution. */
    int width = bus->gpu->width[bus->gpu->status.hres];
    int height = bus->gpu->height[bus->gpu->status.vres];

    /* Display area start. */
    auto& display_area = bus->gpu->display_area;

    /* Ignore scissor test. */
    auto& draw_top_left = bus->gpu->drawing_area_top_left;
    auto& draw_bottom_right = bus->gpu->drawing_area_bottom_right;
    auto size = draw_bottom_right - draw_top_left;
    glScissor(draw_top_left.x, VRAM_HEIGHT - draw_bottom_right.y, size.x, size.y);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);

    /* Immediately draw the provided data. */
    glBindVertexArray(draw_vao);
    glBindBuffer(GL_ARRAY_BUFFER, draw_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(Vertex), data.data());

    shader->bind();
    vram.bind_vram_texture();
    int count = (int)data.size();

    glDrawArrays(GL_TRIANGLES, 0, count);
}

void Renderer::update()
{
    glfwPollEvents();

    /* Clip pixels outside of draw area for the next frame. */
    auto& draw_top_left = bus->gpu->drawing_area_top_left;
    auto& draw_bottom_right = bus->gpu->drawing_area_bottom_right;
    auto size = draw_bottom_right - draw_top_left;
    glScissor(draw_top_left.x, VRAM_HEIGHT - draw_bottom_right.y, size.x, size.y);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    /* Get current display resolution. */
    int width = bus->gpu->width[bus->gpu->status.hres];
    int height = bus->gpu->height[bus->gpu->status.vres];

    /* Display area start. */
    auto& display_area = bus->gpu->display_area;

    glBindVertexArray(draw_vao);
    glBindBuffer(GL_ARRAY_BUFFER, draw_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, draw_data.size() * sizeof(Vertex), draw_data.data());

    shader->bind();
    vram.bind_vram_texture();
    int count = (int)draw_data.size();

    assert(count < MAX_VERTICES);

    glDrawArrays(GL_TRIANGLES, 0, count);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

    /* Remove scissor to copy framebuffer. */
    glScissor(0, 0, VRAM_WIDTH, VRAM_HEIGHT);

    /* Copy from framebuffer to the default framebuffer. */
    glBlitFramebuffer(display_area.x, VRAM_HEIGHT - height - display_area.y + 1, display_area.x + width - 1, VRAM_HEIGHT - display_area.y, 0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    //glBlitFramebuffer(0, 0, VRAM_WIDTH, VRAM_HEIGHT, 0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::swap()
{
    glfwSwapBuffers(window);
    glClear(GL_COLOR_BUFFER_BIT);

    draw_data.clear();
    primitive_count = 0;
}

bool Renderer::is_open()
{
    return !glfwWindowShouldClose(window);
}