#pragma once
#include <string>

class Widget {
public:
	Widget(const std::string& _name) : name(_name) {}
	~Widget() = default;

	const std::string& get_name() const { return name; }
	virtual void execute() = 0;

protected:
	std::string name;
};