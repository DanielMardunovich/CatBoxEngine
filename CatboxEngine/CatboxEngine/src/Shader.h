#pragma once
#include <string>
#include <glm/glm.hpp>

class Shader
{
private:
	std::string LoadShader(const char* aPath);
	unsigned int LoadVertexShader(const char* aShaderPath, unsigned int aShaderType);
	unsigned int LoadFragmentShader(const char* aShaderPath, unsigned int aShaderType);
public:
	unsigned int myShaderProgram;
	void Initialize(const char* aVertexPath, const char* aFragmentPath);
	void Use();

	// Upload a 4x4 matrix uniform to the currently used shader program
	void SetMat4(const std::string& name, const glm::mat4& mat);

	void DumpActiveUniforms() const;
};

