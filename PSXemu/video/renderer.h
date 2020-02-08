#pragma once
#include "opengl/buffer.h"
#include "opengl/shader.h"
#include <memory>

using std::unique_ptr;

typedef std::vector<Pos2> Verts;
typedef std::vector<Color> Colors;

class GLFWwindow;
class Renderer {
public:
	Renderer(int width, int height, std::string title);
	~Renderer();

	void push_triangle(Verts& pos, Colors& colors);
	void push_quad(Verts& pos, Colors& colors);
	void draw_scene();

	void set_draw_offset(int16_t x, int16_t y);

	void update();
	bool is_open();

	static void resize_func(GLFWwindow* window, int width, int height);

public:
	GLFWwindow* window;
	int32_t width, height;
	uint32_t vao, count;

	unique_ptr<Shader> shader;
	unique_ptr<Buffer<Color>> color_buffer;
	unique_ptr<Buffer<Pos2>> vertex_buffer;
};                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                