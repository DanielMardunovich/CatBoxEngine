#include "Shader.h"
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

std::string Shader::LoadShader(const char* aPath)
{
    std::ifstream shaderFile(aPath); // Initialize with file path

    if (!shaderFile.is_open()) { //Handle error if no file exists
		std::cerr << "Failed to open shader file: " << aPath << '\n';
        return "";
    }

    std::stringstream shaderStream;
    shaderStream << shaderFile.rdbuf();

    shaderFile.close();
    std::string shaderCode = shaderStream.str();
    return shaderCode;
}

unsigned int Shader::LoadVertexShader(const char* aShaderPath, unsigned int aShaderType)
{
	int result;
    char Log[512];

	std::string shaderCodeString = LoadShader(aShaderPath);
	const char* shaderCode = shaderCodeString.c_str();

    unsigned int shaderObject = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(shaderObject, 1, &shaderCode, nullptr);
    glCompileShader(shaderObject);

	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &result);

    if (!result) {
        glGetShaderInfoLog(shaderObject, 512, nullptr, Log);
		std::cerr << "Vertex Shader Compilation Failed: " << Log << '\n';

    }
    return shaderObject;
}

unsigned int Shader::LoadFragmentShader(const char* aShaderPath, unsigned int aShaderType)
{
    int result;
    char Log[512];

    std::string shaderCodeString = LoadShader(aShaderPath);
    const char* shaderCode = shaderCodeString.c_str();

    unsigned int shaderObject = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shaderObject, 1, &shaderCode, nullptr);
    glCompileShader(shaderObject);

    glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &result);

    if (!result) {
        glGetShaderInfoLog(shaderObject, 512, nullptr, Log);
        std::cerr << "Vertex Shader Compilation Failed: " << Log << '\n';

    }
    return shaderObject;
}

void Shader::Initialize(const char* aVertexPath, const char* aFragmentPath)
{
    int result;
    char Log[512];

    unsigned int vertexShader = LoadVertexShader(aVertexPath, GL_VERTEX_SHADER);
    unsigned int fragmentShader = LoadFragmentShader(aFragmentPath, GL_FRAGMENT_SHADER);
    
    myShaderProgram = glCreateProgram();

    glAttachShader(myShaderProgram, vertexShader);
    glAttachShader(myShaderProgram, fragmentShader);
    glLinkProgram(myShaderProgram);

	glGetProgramiv(myShaderProgram, GL_LINK_STATUS, &result);
    if (!result) {
		glGetProgramInfoLog(myShaderProgram, 512, nullptr, Log);
		std::cerr << "Shader Program Linking Failed: " << Log << '\n';
    }

    glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(myShaderProgram, name.c_str()), (int)value);
}

void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(myShaderProgram, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(myShaderProgram, name.c_str()), value);
}

void Shader::setColor(float r, float g, float b) const
{
    GLint location = glGetUniformLocation(myShaderProgram, "col");
    glUniform3f(location, r, g, b);
    
}

void Shader::SetTexture(const std::string& name, int unit) const
{
    GLint loc = glGetUniformLocation(myShaderProgram, name.c_str());
    if (loc != -1)
    {
        glUniform1i(loc, unit);
    }
}

void Shader::SetBool(const std::string& name, bool value) const
{
    GLint loc = glGetUniformLocation(myShaderProgram, name.c_str());
    if (loc != -1) glUniform1i(loc, value ? 1 : 0);
}

void Shader::setVec3(const std::string& name, float x, float y, float z) const
{
    GLint loc = glGetUniformLocation(myShaderProgram, name.c_str());
    glUniform3f(loc, x, y, z);
}

void Shader::SetMat4(const std::string& name, const glm::mat4& mat) const
{
    GLint loc = glGetUniformLocation(myShaderProgram, name.c_str());
    if (loc == -1)
    {
        std::cerr << "Uniform not found: " << name << '\n';
        return;
    }

    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}


void Shader::Use() const
{
	glUseProgram(myShaderProgram);
}
