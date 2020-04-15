#pragma once
#include <utility/types.hpp>
#include <string>

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

    void set_int(const char* str, int val);
    void set_vec2(const char* str, const glm::vec2& val);

    void load(const std::string& filepath, ShaderType type);
    void build();

    uint raw();

private:
    void check_errors(uint shader, ShaderType type);

private:
    uint shader_id;
    uint vertex, fragment;
};