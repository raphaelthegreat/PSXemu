#pragma once
#include <cstdint>
#include <vector>
#include <utility/types.hpp>
#include <glad/glad.h>

enum class Format {
	RGB = 0x1907,
	RGBA = 0x1908,
	PACKED5551 = 0x8034,
	PACKED1555_REV = 0x8366
};

enum class Filtering {
	Linear = 0x2601,
	Nearest = 0x2600
};

/* Base texture wrapper. */
template <typename T = ubyte>
class Texture {
public:
	Texture() = default;
	Texture(uint _width, uint _height,
			T* pixels = NULL,
			Filtering filtering = Filtering::Nearest,
			Format format = Format::RGBA);
	~Texture();

	void bind();
	void unbind();

	void recreate(uint _width, uint _height, 
				  T* pixels = NULL);
	void update(T* pixels);

public:
	std::vector<ubyte> pixels;
	
	uint width, height;
	uint texture_id;

	Format tformat;
	Filtering tfiltering;
};

typedef Texture<ubyte> Texture8;

template<typename T>
Texture<T>::Texture(uint _width, uint _height, T* pixels, Filtering filtering, Format format)
{
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	/* Set the texture wrapping parameters. */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	/* Set texture filtering parameters. */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)filtering);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)filtering);

	width = _width; height = _height;

	int type = GL_UNSIGNED_BYTE;
	if constexpr (std::is_same_v<T, int32_t>)
		type = GL_INT;
	else if constexpr (std::is_same_v<T, ushort>)
		type = GL_UNSIGNED_SHORT;

	/* Allocate space on the GPU. */
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, (int)format, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	tfiltering = filtering;
	tformat = format;

	unbind();
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

template<typename T>
inline void Texture<T>::recreate(uint _width, uint _height, T* pixels)
{
	bind();
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
									_width, 
									_height, 
									0, 
									(GLenum)tformat, 
									GL_UNSIGNED_BYTE, 
									pixels);

	unbind();
}

template<typename T>
inline void Texture<T>::update(T* pixels)
{
	bind();
	
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
									width, 
									height, 
									(GLenum)tformat, 
									GL_UNSIGNED_BYTE, 
									pixels);

	unbind();
}
