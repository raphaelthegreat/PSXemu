#pragma once
#include <string>
#include <GLFW/glfw3.h>
#include <fstream>

class CPU;
class GPU;
class GUI {
public:
	GUI() = default;
	~GUI() = default;
	
	void init(GLFWwindow* _window);

	void start();
	void display();

	void dockspace();
	void decompiler(CPU* cpu);
	void viewport(uint32_t texture);
	void cpu_regs(CPU* cpu);

public:
	GLFWwindow* window;
	std::ofstream log;
	bool should_log = false;
	bool display_vram = true;
};