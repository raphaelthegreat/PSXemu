#pragma once
#include <vector>
#include "cpu_widget.hpp"
#include "mem_widget.hpp"

class Bus;
class Debugger {
public:
	friend class CPUWidget;
	friend class MemWidget;
	friend class GPUWidget;

public:
	Debugger(Bus* _bus);
	~Debugger();

	void init_theme();
	void display();

	template <typename T>
	void push_widget();

private:
	Bus* bus;
	std::vector<Widget*> widget_stack;
};

template<typename T>
inline void Debugger::push_widget()
{
	widget_stack.push_back(new T(this));
}
