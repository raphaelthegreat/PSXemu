#pragma once
#include <vector>
#include "cpu_widget.hpp"

struct GLFWwindow;
class Debugger {
public:
	Debugger(GLFWwindow* _window);
	~Debugger();

	void push_widget(Widget* w);
	void display();

private:
	GLFWwindow* window;
	std::vector<Widget*> widget_stack;
};