#pragma once
#include <string>
#include <glm/fwd.hpp>

class Shader
{
private:
    static std::string LoadShader(const char* aPath);
    static unsigned int LoadVertexShader(const char* aShaderPath, unsigned int aShaderType);
    static unsigned int LoadFragmentShader(const char* aShaderPath, unsigned int aShaderType);
public:
    void Initialize(const char* aVertexPath, const char* aFragmentPath);

    // utility uniform functions
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;

    void setColor(float r, float g, float b) const;

    void setVec3(const std::string& name, float x, float y, float z) const;
    void SetMat4(const std::string& name, const glm::mat4& mat) const;


    void Use() const;
    
    unsigned int myShaderProgram;
};
