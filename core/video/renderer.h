#pragma once
#include "opengl/shader.h"
#include <video/gpu_core.h>
#include <GLFW/glfw3.h>

constexpr int MAX_VERTICES = 1024 * 512;

enum class Primitive {
	Polygon = 0,
	Rectangle = 1,
	Line = 2
};

class GPU;
class Bus;
class Renderer {
public:
	Renderer(int width, int height, const std::string& title, Bus* _bus);
	~Renderer();

	/* Batch vertex data. */
	void draw_call(std::vector<Vertex>& data, Primitive primitive);
	/* Force draw vertex data. */
	void draw(std::vector<Vertex>& data);

	void update();
	void swap();
	bool is_open();

public:
	int32_t window_width, window_height;
	uint framebuffer;
	uint framebuffer_texture;
	uint framebuffer_rbo;

	uint draw_vbo, draw_vao;
	uint primitive_count = 0;
	std::vector<Vertex> draw_data;

	std::unique_ptr<Shader> shader;
	GLFWwindow* window;
	Bus* bus;
};                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             