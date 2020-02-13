#include "texture.h"
#include <glad/glad.h>

void TextureBuffer::push_data(uint32_t data)
{
    uint16_t pixel0 = unpack((uint16_t)data);
    uint16_t pixel1 = unpack((uint16_t)(data >> 16));

    //pixels.push_back(pixel0);
    //pixels.push_back(pixel1);
}

uint16_t TextureBuffer::unpack(uint16_t color)
{
    auto a = color >> 15;
    auto r = (color >> 10) & 0x1f;
    auto g = (color >> 5) & 0x1f;
    auto b = color & 0x1f;
    
    uint8_t red = (255 / 32 * r);
    uint8_t green = (255 / 32 * g);
    uint8_t blue = (255 / 32 * b);
    uint8_t alpha = (a * 255);

    pixels.push_back(red);
    pixels.push_back(green);
    pixels.push_back(blue);
    pixels.push_back(alpha);

    return (alpha | (r << 1) | (g << 6) | (b << 11));
}

Texture TextureBuffer::texture()
{
    Texture tex;
    tex.width = width;
    tex.height = height;

    glGenTextures(1, &tex.texture_id);
    glBindTexture(GL_TEXTURE_2D, tex.texture_id);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels.front());
    glGenerateMipmap(GL_TEXTURE_2D);

    return tex;
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