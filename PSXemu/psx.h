#pragma once
#include <cpu/cpu.h>
#include <video/gpu_core.h>

class Renderer;
struct GLFWwindow;
class PSX {
public:
	PSX(Renderer* renderer);
	~PSX() = default;

	void tick();
	static void key_callback(GLFWwindow* window, int key, 
										  int scancode, 
										  int action, 
										  int mods);

public:
	unique_ptr<CPU> cpu;
	unique_ptr<Bus> bus;
	unique_ptr<GPU> gpu;

	Renderer* gl_renderer;
	GLFWwindow* window;
};