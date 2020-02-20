#pragma once
#include "opengl/buffer.h"
#include "opengl/shader.h"
#include "opengl/texture.h"
#include <unordered_map>
#include <memory>

using std::unique_ptr;

typedef std::vector<Pos2i> Verts;
typedef std::vector<Pos2f> Coords;
typedef std::vector<Color8> Colors;
typedef std::vector<uint8_t> Pixels;

class GLFWwindow;
class Renderer {
public:
	Renderer(int width, int height, std::string title);
	~Renderer();

	void push_triangle(Verts& pos, Colors& colors);
	void push_quad(Verts& pos, Colors& colors);
	void push_textured_quad(Verts& pos, Coords& coords, Texture8* texture);
	
	void set_draw_offset(int16_t x, int16_t y);
	void draw_scene();

	void update();
	bool is_open();

	static void resize_func(GLFWwindow* window, int width, int height);

public:
	GLFWwindow* window;
	int32_t width, height;
	uint32_t offsetx, offsety;
	uint32_t fbo, color_buffer;
};                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                