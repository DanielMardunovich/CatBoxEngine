#pragma once
#include <string>

class Shader
{
private:
	std::string LoadShader(const char* aPath);
	unsigned int LoadVertexShader(const char* aShaderPath, unsigned int aShaderType);
	unsigned int LoadFragmentShader(const char* aShaderPath, unsigned int aShaderType);
public:
	unsigned int myShaderProgram;
	void Initialize(const char* aVertexPath, const char* aFragmentPath);

	// utility uniform functions
	void setBool(const std::string& name, bool value) const;
	void setInt(const std::string& name, int value) const;
	void setFloat(const std::string& name, float value) const;

	void setColor(float r, float g, float b) const;

	void setVec3(const std::string& name, float x, float y, float z) const;

	void Use();
};

