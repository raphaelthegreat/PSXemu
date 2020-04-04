#include <stdafx.hpp>
#include "psx.h"
#include "video/renderer.h"

PSX::PSX(Renderer* renderer)
{
	gl_renderer = renderer;
	bus = std::make_unique<Bus>("./bios/SCPH1001.BIN", gl_renderer);
	cpu = std::make_unique<CPU>(bus.get());
	gpu = std::make_unique<GPU>(gl_renderer);

	/* Attach devices to the bus. */
	bus->cpu = cpu.get();
	bus->gpu = gpu.get();
	gl_renderer->gpu = gpu.get();
	
	window = gl_renderer->window;
	glfwSetWindowUserPointer(window, bus.get());
	glfwSetKeyCallback(window, &PSX::key_callback);
}

void PSX::tick()
{	
	for (int i = 0; i < 100; i++) {
		cpu->tick();
	}

	cpu->handle_interrupts();
	bus->tick();
}

void PSX::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Bus* bus = (Bus*)glfwGetWindowUserPointer(window);
	
	if (action == GLFW_PRESS) {
		bus->controller.controller.key_down(key);
		
		if (key == GLFW_KEY_F1) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		else if (key == GLFW_KEY_F2) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glClear(GL_COLOR_BUFFER_BIT);
		}
	}
	else if (action == GLFW_RELEASE) {
		bus->controller.controller.key_up(key);
	}
}
