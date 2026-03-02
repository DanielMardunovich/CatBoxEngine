#include "Shader.h"
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

std::string Shader::LoadShaderSource(const char* path)
{
    std::ifstream shaderFile(path);

    if (!shaderFile.is_open())
    {
        std::cerr << "Failed to open shader file: " << path << '\n';
        return "";
    }

    std::stringstream shaderStream;
    shaderStream << shaderFile.rdbuf();
    shaderFile.close();

    return shaderStream.str();
}

unsigned int Shader::CompileShader(const char* shaderPath, unsigned int shaderType)
{
    const std::string shaderCodeString = LoadShaderSource(shaderPath);
    const char* shaderCode = shaderCodeString.c_str();

    const unsigned int shaderObject = glCreateShader(shaderType);
    glShaderSource(shaderObject, 1, &shaderCode, nullptr);
    glCompileShader(shaderObject);

    int result;
    glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &result);

    if (!result)
    {
        char log[512];
        glGetShaderInfoLog(shaderObject, 512, nullptr, log);
        const char* shaderTypeName = (shaderType == GL_VERTEX_SHADER) ? "Vertex" : "Fragment";
        std::cerr << shaderTypeName << " Shader Compilation Failed: " << log << '\n';
    }

    return shaderObject;
}

void Shader::Initialize(const char* vertexPath, const char* fragmentPath)
{
    const unsigned int vertexShader = CompileShader(vertexPath, GL_VERTEX_SHADER);
    const unsigned int fragmentShader = CompileShader(fragmentPath, GL_FRAGMENT_SHADER);
    
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);

    int result;
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &result);
    if (!result)
    {
        char log[512];
        glGetProgramInfoLog(m_shaderProgram, 512, nullptr, log);
        std::cerr << "Shader Program Linking Failed: " << log << '\n';
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Shader::SetBool(const std::string& name, bool value) const
{
    const GLint loc = glGetUniformLocation(m_shaderProgram, name.c_str());
    if (loc != -1)
    {
        glUniform1i(loc, value ? 1 : 0);
    }
}

void Shader::SetInt(const std::string& name, int value) const
{
    const GLint loc = glGetUniformLocation(m_shaderProgram, name.c_str());
    if (loc != -1)
    {
        glUniform1i(loc, value);
    }
}

void Shader::SetFloat(const std::string& name, float value) const
{
    const GLint loc = glGetUniformLocation(m_shaderProgram, name.c_str());
    if (loc != -1)
    {
        glUniform1f(loc, value);
    }
}

void Shader::SetVec3(const std::string& name, float x, float y, float z) const
{
    const GLint loc = glGetUniformLocation(m_shaderProgram, name.c_str());
    if (loc != -1)
    {
        glUniform3f(loc, x, y, z);
    }
}

void Shader::SetColor(float r, float g, float b) const
{
    const GLint location = glGetUniformLocation(m_shaderProgram, "col");
    if (location != -1)
    {
        glUniform3f(location, r, g, b);
    }
}

void Shader::SetTexture(const std::string& name, int unit) const
{
    const GLint loc = glGetUniformLocation(m_shaderProgram, name.c_str());
    if (loc != -1)
    {
        glUniform1i(loc, unit);
    }
}

void Shader::SetMat4(const std::string& name, const glm::mat4& mat) const
{
    if (m_shaderProgram == 0) return;

    const GLint loc = glGetUniformLocation(m_shaderProgram, name.c_str());
    if (loc != -1)
    {
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
    }
}

void Shader::SetMat4Array(const std::string& name, const glm::mat4* mats, int count) const
{
    if (m_shaderProgram == 0 || count <= 0) return;

    const GLint loc = glGetUniformLocation(m_shaderProgram, name.c_str());
    if (loc != -1)
    {
        glUniformMatrix4fv(loc, count, GL_FALSE, glm::value_ptr(mats[0]));
    }
}

void Shader::Use() const
{
    glUseProgram(m_shaderProgram);
}
