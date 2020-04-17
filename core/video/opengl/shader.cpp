#include <stdafx.hpp>
#include "shader.h"
#include <glad/glad.h>

Shader::~Shader()
{
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glDeleteProgram(shader_id);
}

void Shader::bind()
{
    glUseProgram(shader_id);
}

void Shader::unbind()
{
    glUseProgram(0);
}

void Shader::set_int(const char* str, int val)
{
    auto loc = glGetUniformLocation(shader_id, str);
    glUniform1i(loc, val);
    
}

void Shader::set_vec2(const char* str, const glm::vec2& val)
{
    auto loc = glGetUniformLocation(shader_id, str);
    glUniform2fv(loc, 1, glm::value_ptr(val));
}

void Shader::load(const std::string& filepath, ShaderType type)
{
    std::string shader_code;
    std::ifstream shader_file;

    shader_file.exceptions(std::ifstream::failbit | 
                           std::ifstream::badbit);
    try {
        shader_file.open(filepath);
        
        std::stringstream ss;
        ss << shader_file.rdbuf();
       
        shader_file.close();
        shader_code = ss.str();        
    }
    catch (std::ifstream::failure e) {
        printf("Could not read shader file!\n");
    }
    
    const char* shader_str = shader_code.c_str();

    uint shader = glCreateShader((GLenum)type);
    glShaderSource(shader, 1, &shader_str, NULL);
    glCompileShader(shader);
    
    check_errors(shader, type);

    if (type == ShaderType::Vertex)
        vertex = shader;
    else if (type == ShaderType::Fragment)
        fragment = shader;
}

void Shader::build()
{
    shader_id = glCreateProgram();

    glAttachShader(shader_id, vertex);
    glAttachShader(shader_id, fragment);

    glLinkProgram(shader_id);
    glUseProgram(shader_id);
}

uint Shader::raw()
{
    return shader_id;
}

void Shader::check_errors(uint shader, ShaderType type)
{
    GLint success;
    GLchar info_log[1024];
    
    const char* str = (type == ShaderType::Vertex ? "vertex" : "fragment");

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, info_log);
        printf("Could not compile %s shader!\n", str); 
        printf("%s\n", info_log);
        printf("-------------------------------------\n");
    }
}
