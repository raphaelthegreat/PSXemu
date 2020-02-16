#pragma once
#include <cstdint>
#include <vector>
#include <glad/glad.h>

enum class Format {
	RGB = 0x1907,
	RGBA = 0x1908,
	PACKED5551 = 0x8034
};

enum class Filtering {
	Linear = 0x2601,
	Nearest = 0x2600
};

/* A struct used to indentify a texture. */
/* Using it's info it produces a hash unique */
/* to each texture. */
struct TextureInfo {
	uint32_t width, height;
	uint32_t vram_x, vram_y;
	Format format;

	bool operator==(const TextureInfo& info) const {
		return (width == info.width) &&
			(height == info.height) &&
			(vram_x == info.vram_x) &&
			(vram_y == info.vram_y) &&
			(format == info.format);
	}
};

struct TextureHash {
	std::size_t operator()(const TextureInfo info) const {
		size_t res = 17;
		
		res = res * 31 + std::hash<uint32_t>()(info.width);
		res = res * 31 + std::hash<uint32_t>()(info.height);
		res = res * 31 + std::hash<uint32_t>()(info.vram_x);
		res = res * 31 + std::hash<uint32_t>()(info.vram_y);
		res = res * 31 + std::hash<Format>()(info.format);
		return res;
	}
};

/* Base texture wrapper. */
template <typename T = uint8_t>
class Texture {
public:
	Texture() = default;
	Texture(uint32_t _width, uint32_t _height,
			T* pixels = NULL,
			Format format = Format::RGBA,
			Filtering filtering = Filtering::Nearest);
	~Texture();

	void bind();
	void unbind();

public:
	std::vector<uint8_t> pixels;
	
	uint32_t width, height;
	uint32_t texture_id;
};

typedef Texture<uint8_t> Texture8;

template<typename T>
Texture<T>::Texture(uint32_t _width, uint32_t _height, T* pixels, Format format, Filtering filtering)
{
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	/* Set the texture wrapping parameters. */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	/* Set texture filtering parameters. */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)filtering);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)filtering);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	width = _width; height = _height;

	int type = GL_UNSIGNED_BYTE;
	if constexpr (std::is_same_v<T, int32_t>)
		type = GL_INT;
	else if constexpr (std::is_same_v<T, uint16_t>)
		type = GL_UNSIGNED_SHORT;

	/* Allocate space on the GPU. */
	glTexImage2D(GL_TEXTURE_2D, 0, (int)format, width, height, 0, (int)format, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
}

template<typename T>
Texture<T>::~Texture()
{
	/* Deallocate GPU memory. */
	glDeleteTextures(1, &texture_id);
}

template <typename T>
void Texture<T>::bind()
{
	glBindTexture(GL_TEXTURE_2D, texture_id);
}

template <typename T>
void Texture<T>::unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}