#pragma once
#include <glad/glad.h>
#include <cpu/util.h>
#include <cstdint>
#include <vector>

#define BUFFER_SIZE 64 * 1024

/* Vertex color attribute. */
class Color {
public:
	Color() = default;
	Color(uint8_t r, uint8_t g, uint8_t b) :
		red(r), green(g), blue(b) {}

	static Color from_gpu(uint32_t val);

public:
	uint8_t red = 0, green = 0, blue = 0;
};

/* Vertex position attribute. */
class Pos2 {
public:
	Pos2() = default;
	Pos2(int16_t _x, int16_t _y) : x(_x), y(_y) {}

	static Pos2 from_gpu(uint32_t val);

public:
	int16_t x = 0, y = 0;
};

/* An OpenGL VBO that contains the vertices. */
template <typename T>
class Buffer {
public:
	/* Constructs a vertex buffer from data. */
	Buffer();
	~Buffer();

	void bind() { glBindBuffer(GL_ARRAY_BUFFER, vbo); }
	void unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }
	
	/* Sets the buffer contents. */
	void set(uint32_t index, T val);

public:
	uint32_t vbo;
	T* buffer_ptr;
};

template<typename T>
inline Buffer<T>::Buffer()
{
	GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;
	glGenBuffers(1, &vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(T) * BUFFER_SIZE, nullptr, flags);

	buffer_ptr = (T*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(T) * BUFFER_SIZE, flags);
}

template<typename T>
inline Buffer<T>::~Buffer()
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glDeleteBuffers(1, &vbo);
}

template<typename T>
inline void Buffer<T>::set(uint32_t index, T val)
{
	if (index >= BUFFER_SIZE)
		panic("Exceeded max buffer size!", "");

	buffer_ptr[index] = val;
}
