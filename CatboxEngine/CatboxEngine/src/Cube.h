#pragma once
#include "Renderable.h"
#include <glad/glad.h>
#include <glm/glm.hpp>

class Cube : public Renderable
{
public:
    Cube();
    explicit Cube(const glm::vec3& position);
    ~Cube();

    void Render(Shader& aShader) override;

    // Update this cube's model transform (position in world + Euler rotation in degrees)
    void SetModel(const glm::vec3& position, const glm::vec3& rotationDeg);

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    glm::mat4 m_Model = glm::mat4(1.0f);

    void SetupMesh();
};