#pragma once
#include <cstdint>
#include <vector>

class Texture {
public:
	Texture() = default;
	Texture(uint32_t _width, uint32_t _height);
	~Texture();

	void bind();
	void unbind();

public:
	std::vector<uint8_t> pixels;
	
	uint32_t width, height;
	uint32_t texture_id;
};

class TextureBuffer {
public:
	TextureBuffer() = default;

	/* Push pixel data to buffer. */
	void push_data(uint32_t data);

	/* Upack pixel data. */
	uint16_t unpack(uint16_t color);

	/* Generates a texture. */
	Texture texture();

public:
	std::vector<uint16_t> pixels;
	std::pair<uint16_t, uint16_t> top_left = std::make_pair(0, 0);

	uint32_t width = 0, height = 0;
};