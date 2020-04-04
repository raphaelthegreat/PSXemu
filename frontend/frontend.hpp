#pragma once
#include <window.hpp>

class Frontend : public wxApp {
public:
	Frontend();
	~Frontend();

	bool OnInit() override;

private:
	Window* window_frame = nullptr;
};