#pragma once
#include "opengl/shader.h"
#include "opengl/texture.h"
#include <GLFW/glfw3.h>
#include <memory>
#include <debug/gui.h>

using std::unique_ptr;

class VRAM;
class Bus;
class Renderer {
public:
	Renderer(int width, int height, std::string title);
	~Renderer();

	void set_draw_offset(int16_t x, int16_t y);
	void draw_scene();

	void update(Bus* bus);
	bool is_open();

	static void resize_func(GLFWwindow* window, int width, int height);

public:
	GLFWwindow* window;
	int32_t width, height;
	uint32_t offsetx, offsety;
	uint32_t screen_vao, screen_vbo;

	/* Shader and screen texture. */
	unique_ptr<Shader> shader;
	unique_ptr<Texture8> screen_texture;

	/* Screen pixels. */
	std::vector<uint8_t> pixels;

	GUI debug_gui;
};                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             