#include "psx.h"
#include <cpu/cpu.h>
#include "video/renderer.h"

//#pragma optimize("", off)

PSX::PSX(Renderer* renderer)
{
	gl_renderer = renderer;
	bus = std::make_unique<Bus>("SCPH1001.BIN", gl_renderer);
	cpu = std::make_unique<CPU>(bus.get());
	gpu = std::make_unique<GPU>();

	/* Attach devices to the bus. */
	bus->cpu = cpu.get();
	bus->gpu = gpu.get();
	
	window = gl_renderer->window;
	glfwSetWindowUserPointer(window, bus.get());
	glfwSetKeyCallback(window, &PSX::key_callback);
}

void PSX::tick()
{	
	for (int i = 0; i < 100; i++) {
		cpu->tick();
	}

	bus->tick();
	cpu->handle_interrupts();
}

void PSX::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Bus* bus = (Bus*)glfwGetWindowUserPointer(window);
	
	if (action == GLFW_PRESS) {
		bus->controller.controller.handleJoyPadDown(key);
		
		if (key == GLFW_KEY_TAB)
			bus->gl_renderer->debug_gui.display_vram = !bus->gl_renderer->debug_gui.display_vram;
	}
	else if (action == GLFW_RELEASE) {
		bus->controller.controller.handleJoyPadUp(key);
	}
}
