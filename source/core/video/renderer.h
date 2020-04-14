#pragma once
#include "opengl/shader.h"
#include <tools/debugger.hpp>
#include <video/gpu_core.h>
#include <GLFW/glfw3.h>

constexpr int MAX_VERTICES = 8192;

enum class Primitive {
	Polygon = 0,
	Rectangle = 1,
	Line = 2
};

class GPU;
class Bus;
class Renderer {
public:
	Renderer(int width, int height, std::string title);
	~Renderer();

	void draw_call(std::vector<Vertex>& data, Primitive primitive);
	void init_debugger(Bus* bus);

	void update();
	bool is_open();

	static void resize_func(GLFWwindow* window, int width, int height);

public:
	int32_t window_width, window_height;
	uint framebuffer;
	uint framebuffer_texture;
	uint framebuffer_rbo;

	uint draw_vbo, draw_vao;
	uint primitive_count = 0;
	std::vector<Vertex> draw_data;
	std::vector<Primitive> draw_order;
	
	Shader* shader;
	GLFWwindow* window;
	Debugger* debugger;
	GPU* gpu;
};                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             