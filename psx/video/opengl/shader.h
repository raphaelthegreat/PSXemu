#pragma once
#include <glad/glad.h>
#include <string>

#define STR(x) #x

enum class ShaderType {
    Vertex = 0x8B31,
    Fragment = 0X8B30
};

class Shader {
public:
    Shader() = default;
    ~Shader();

    void bind();
    void unbind();

    void load(std::string filepath, ShaderType type);
    void build();

    uint32_t raw();

private:
    void check_errors(GLuint shader, ShaderType type);

private:
    uint32_t shader_id;
    uint32_t vertex, fragment;
};