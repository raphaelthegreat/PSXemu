#pragma once
#include <glad/glad.h>
#include <cpu/util.h>
#include <cstdint>
#include <vector>

#define BUFFER_SIZE 64 * 1024

/* Vertex color attribute. */
template <typename T = uint8_t>
class Color {
public:
	Color() = default;
	Color(T r, T g, T b, T a) :
		red(r), green(g), blue(b), alpha(a) {}
	Color(T r, T g, T b) : red(r), green(g), blue(b) {}

	static Color<uint8_t> from_gpu(uint32_t val);

public:
	T red = 0, green = 0, blue = 0, alpha = 0;
};

typedef Color<uint8_t> Color8;
typedef Color<float> Colorf;

/* Vertex position attribute. */
template <typename T = int16_t>
class Pos2 {
public:
	Pos2() = default;
	Pos2(T _x, T _y) : x(_x), y(_y) {}

	static Pos2<int16_t> from_gpu(uint32_t val);

public:
	T x = 0, y = 0;
};

typedef Pos2<int16_t> Pos2i;
typedef Pos2<float> Pos2f;

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
	GLbitfield flags = GL_MAP_WRITE_BIT ;
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

template<typename T>
inline Color<uint8_t> Color<T>::from_gpu(uint32_t val)
{
	Color color;
	color.red = (uint8_t)val;
	color.green = (uint8_t)(val >> 8);
	color.blue = (uint8_t)(val >> 16);

	return color;
}

template<typename T>
inline Pos2<int16_t> Pos2<T>::from_gpu(uint32_t val)
{
	Pos2i pos;
	pos.x = (int16_t)bit_range(val, 0, 16);
	pos.y = (int16_t)bit_range(val, 16, 32);

	return pos;
}