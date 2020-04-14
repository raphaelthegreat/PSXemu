﻿#include <stdafx.hpp>
#include "renderer.h"

#include <memory/bus.h>
#include <video/vram.h>
#include <glad/gl.h>

Renderer::Renderer(int width, int height, std::string title)
{
    window_width = width;
    window_height = height;

	glfwInit();
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    glfwMakeContextCurrent(window);

    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        printf("[OpenGL] gladLoadGL: failed to load OpenGL.\n");
        exit(1);
    }

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glfwSetFramebufferSizeCallback(window, resize_func);

    /* Build shader */
    shader = new Shader();
    shader->load("data/shaders/vertex.vert", ShaderType::Vertex);
    shader->load("data/shaders/fragment.frag", ShaderType::Fragment);
    shader->build();

    /* Create the screen framebuffer. */
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &framebuffer_texture);
    glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer_texture, 0);

    glGenRenderbuffers(1, &framebuffer_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, framebuffer_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebuffer_rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("[OpenGL] glCheckFramebufferStatus: failed to create screen framebuffer.\n");
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
    delete shader;

    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &framebuffer_texture);

    glDeleteBuffers(1, &draw_vbo);
    glDeleteVertexArrays(1, &draw_vao);

    glfwTerminate();
}

void Renderer::draw_call(std::vector<Vertex>& data, Primitive p)
{
    draw_data.insert(draw_data.end(), data.begin(), data.end());
    draw_order.push_back(p);
}

void Renderer::init_debugger(Bus* bus)
{
    //debugger = new Debugger(window);
    //debugger->push_widget(new CPUWidget(bus->cpu));
}

void Renderer::update()
{
    glfwPollEvents();

    /* Clip pixels outside of draw area for the next frame. */
    auto& top_left = gpu->drawing_area_top_left;
    auto& bottom_right = gpu->drawing_area_bottom_right;
    auto size = bottom_right - top_left;
    glScissor(top_left.x, 512 - bottom_right.y - 1, size.x + 1, size.y + 1);

    /* Draw the scene. */
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);

    glBindVertexArray(draw_vao);
    glBindBuffer(GL_ARRAY_BUFFER, draw_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, draw_data.size() * sizeof(Vertex), draw_data.data());

    shader->bind();
    vram.bind_vram_texture();
    glDrawArrays(GL_TRIANGLES, 0, draw_data.size());

    /* Revert the scissor to copy the framebuffer. */
    glScissor(0, 0, window_width, window_height);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    
    glClear(GL_COLOR_BUFFER_BIT);
    glBlitFramebuffer(0, 0, window_width, window_height, 0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);

    glfwSwapBuffers(window);

    draw_data.clear();
    primitive_count = 0;
}

bool Renderer::is_open()
{
	return !glfwWindowShouldClose(window);
}

void Renderer::resize_func(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}