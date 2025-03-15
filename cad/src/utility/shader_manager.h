#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

using ShaderProgram = unsigned int;

struct ShaderSource {
    std::string vs; // vertex shader
    std::string fs; // fragment shader
};

struct ShaderManager {
    fs::path shaders_dir_path = {"../shaders/"};

    [[nodiscard]]
    ShaderProgram shader_program(const std::string& shader_name) const {
        const auto shader_source = loadShaderSource(shader_name);
        return compileShader(shader_source);
    }

    [[nodiscard]]
    static ShaderProgram compileShader(const ShaderSource& shader_source) {
        const char* vertexShaderCStr = shader_source.vs.c_str();
        const char* fragmentShaderCStr = shader_source.fs.c_str();

        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderCStr, NULL);
        glCompileShader(vertexShader);

        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
        }

        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderCStr, NULL);
        glCompileShader(fragmentShader);

        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
        }

        unsigned int shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            std::cerr << "Shader program linking failed: " << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return shaderProgram;
    }

    [[nodiscard]]
    ShaderSource loadShaderSource(const std::string& shader_name) const {
        const auto vs = loadVertexShader(shader_name);
        const auto fs = loadFragmentShader(shader_name);
        ShaderSource shader_source = {vs, fs};
        return shader_source;
    }

    [[nodiscard]]
    std::string loadVertexShader(const std::string& shader_name) const {
        auto full_path = shaders_dir_path / shader_name / "vs.glsl";
        std::ifstream file(full_path);

        if (!file.is_open()) {
            std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << full_path << std::endl;
            std::exit(EXIT_FAILURE); // Panic and exit
        }
        std::stringstream shaderStream;
        shaderStream << file.rdbuf();
        file.close();
        return shaderStream.str();
    }

    [[nodiscard]]
    std::string loadFragmentShader(const std::string& shader_name) const {
        auto full_path = shaders_dir_path / shader_name / "fs.glsl";
        std::ifstream file(full_path);

        if (!file.is_open()) {
            std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << full_path << std::endl;
            std::exit(EXIT_FAILURE); // Panic and exit
        }
        std::stringstream shaderStream;
        shaderStream << file.rdbuf();
        file.close();
        return shaderStream.str();
    }
};
