#include "Shader.h"
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

std::string Shader::LoadShader(const char* aPath)
{
    std::string shaderCode;
    std::ifstream shaderFile(aPath); // Initialize with file path

    if (!shaderFile.is_open()) { //Handle error if no file exists
		std::cerr << "Failed to open shader file: " << aPath << std::endl;
        return "";
    }

    std::stringstream shaderStream;
    shaderStream << shaderFile.rdbuf();

    shaderFile.close();
    shaderCode = shaderStream.str();
    return shaderCode;
}

unsigned int Shader::LoadVertexShader(const char* aShaderPath, unsigned int aShaderType)
{
	int result;
    char Log[512];

	std::string shaderCodeString = LoadShader(aShaderPath);
	const char* shaderCode = shaderCodeString.c_str();

    unsigned int shaderObject;
	shaderObject = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(shaderObject, 1, &shaderCode, NULL);
    glCompileShader(shaderObject);

	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &result);

    if (!result) {
        glGetShaderInfoLog(shaderObject, 512, NULL, Log);
		std::cerr << "Vertex Shader Compilation Failed: " << Log << std::endl;

    }
    return shaderObject;
}

unsigned int Shader::LoadFragmentShader(const char* aShaderPath, unsigned int aShaderType)
{
    int result;
    char Log[512];

    std::string shaderCodeString = LoadShader(aShaderPath);
    const char* shaderCode = shaderCodeString.c_str();

    unsigned int shaderObject;
    shaderObject = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shaderObject, 1, &shaderCode, NULL);
    glCompileShader(shaderObject);

    glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &result);

    if (!result) {
        glGetShaderInfoLog(shaderObject, 512, NULL, Log);
        std::cerr << "Vertex Shader Compilation Failed: " << Log << std::endl;

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
		glGetProgramInfoLog(myShaderProgram, 512, NULL, Log);
		std::cerr << "Shader Program Linking Failed: " << Log << std::endl;
    }

    glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void Shader::Use()
{
	glUseProgram(myShaderProgram);
}

void Shader::SetMat4(const std::string& name, const glm::mat4& mat)
{
    if (myShaderProgram == 0)
    {
        std::cerr << "Shader::SetMat4 called before shader program was created\n";
        return;
    }

    GLint location = glGetUniformLocation(static_cast<GLuint>(myShaderProgram), name.c_str());
    if (location == -1)
    {
        // Not fatal: uniform may be optimized away, but warn to help debugging.
        std::cerr << "Warning: uniform '" << name << "' not found or optimized out in shader\n";
        return;
    }

    glUseProgram(myShaderProgram);
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::DumpActiveUniforms() const
{
    if (myShaderProgram == 0) return;
    GLint count = 0;
    glGetProgramiv(myShaderProgram, GL_ACTIVE_UNIFORMS, &count);
    std::cout << "Active uniforms: " << count << std::endl;
    for (GLint i = 0; i < count; ++i)
    {
        char name[256];
        GLint size; GLenum type;
        GLsizei length;
        glGetActiveUniform(myShaderProgram, (GLuint)i, sizeof(name), &length, &size, &type, name);
        GLint loc = glGetUniformLocation(myShaderProgram, name);
        std::cout << "  " << name << " (loc=" << loc << ", size=" << size << ", type=0x" << std::hex << type << std::dec << ")\n";
    }
}
