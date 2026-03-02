#pragma once
#include <string>
#include <glm/fwd.hpp>

class Shader
{
public:
    Shader() = default;
    ~Shader() = default;

    // Disable copy, enable move
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) noexcept = default;
    Shader& operator=(Shader&&) noexcept = default;

    void Initialize(const char* vertexPath, const char* fragmentPath);
    void Use() const;
    
    [[nodiscard]] unsigned int GetProgram() const noexcept { return m_shaderProgram; }

    // Uniform setters - consistent naming (PascalCase)
    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec3(const std::string& name, float x, float y, float z) const;
    void SetColor(float r, float g, float b) const;
    void SetTexture(const std::string& name, int unit) const;
    void SetMat4(const std::string& name, const glm::mat4& mat) const;
    void SetMat4Array(const std::string& name, const glm::mat4* mats, int count) const;

private:
    static std::string LoadShaderSource(const char* path);
    static unsigned int CompileShader(const char* shaderPath, unsigned int shaderType);

    unsigned int m_shaderProgram = 0;
};
