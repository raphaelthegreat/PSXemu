#include <stdafx.hpp>
#include <glad/glad.h>
#include "opengl/stb_image.h"
#include "opengl/stb_image_write.h"
#include "vram.h"

void VRAM::init()
{
	uint buffer_mode = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT;

	/* --------------------------------------------------------------------- */
	glGenBuffers(1, &pbo);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);

	glBufferStorage(GL_PIXEL_UNPACK_BUFFER, 1024 * 512 * 2, nullptr, buffer_mode);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_RECTANGLE, texture);

	/* Set the texture wrapping parameters. */
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	/* Set texture filtering parameters. */
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	/* Allocate space on the GPU. */
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R16UI, 1024, 512, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, nullptr);

	glBindTexture(GL_TEXTURE_RECTANGLE, texture);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);

	ptr = (ushort*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, 1024 * 512 * 2, buffer_mode);
	/* --------------------------------------------------------------------- */

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	image_buffer = new ubyte[3 * 1024 * 512];
}

void VRAM::upload_to_gpu()
{
	glBindTexture(GL_TEXTURE_RECTANGLE, texture);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 1024, 512, GL_RED_INTEGER, GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void VRAM::bind_vram_texture()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE, texture);
}

void VRAM::write_to_image()
{
	stbi_write_jpg("vram_dump.jpg", 1024, 512, 3, image_buffer, 100);
}

ushort VRAM::read(uint x, uint y)
{
	int index = (y * 1024) + x;
	return ptr[index];
}

void VRAM::write(uint x, uint y, ushort data)
{
	int index = (y * 1024) + x;
	ptr[index] = data;

	image_buffer[index * 3 + 0] = (data << 3) & 0xf8;
	image_buffer[index * 3 + 0] = (data >> 2) & 0xf8;
	image_buffer[index * 3 + 0] = (data >> 7) & 0xf8;
}

VRAM vram;