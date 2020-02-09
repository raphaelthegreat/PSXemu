#include "texture.h"
#include <glad/glad.h>

void TextureBuffer::push_data(uint32_t data)
{
    unpack((uint16_t)data);
    unpack((uint16_t)(data >> 16));
}

void TextureBuffer::unpack(uint16_t color)
{
    auto alpha = color >> 15;
    auto r = (color >> 10) & 0x1f;
    auto g = (color >> 5) & 0x1f;
    auto b = color & 0x1f;

    pixels.push_back(r);
    pixels.push_back(g);
    pixels.push_back(b);
    pixels.push_back(alpha);
}

Texture::Texture(uint32_t _width, uint32_t _height)
{
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    width = _width;
    height = _height;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::Texture(TextureBuffer& buffer)
{
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    width = buffer.width;
    height = buffer.height;

    pixels = buffer.pixels;
    const void* data = (pixels.empty() ? NULL : &pixels.front());

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::~Texture()
{
    glDeleteTextures(1, &texture_id);
}

void Texture::bind()
{
    glBindTexture(GL_TEXTURE_2D, texture_id);
}

void Texture::unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}