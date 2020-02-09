#pragma once
#include <cstdint>
#include <vector>

class TextureBuffer {
public:
	TextureBuffer() = default;

	void push_data(uint32_t data);

private:
	void unpack(uint16_t color);

public:
	std::vector<uint16_t> pixels;
	std::pair<uint16_t, uint16_t> top_left = std::make_pair(0, 0);

	uint32_t width = 0, height = 0;
};

class Texture {
public:
	Texture(uint32_t _width, uint32_t _height);
	Texture(TextureBuffer& buffer);
	~Texture();

	void bind();
	void unbind();

public:
	std::vector<uint16_t> pixels;
	
	uint32_t width, height;
	uint32_t texture_id;
};