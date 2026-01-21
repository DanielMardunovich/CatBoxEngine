#pragma once
#include "../Entity.h"
#include <glad/glad.h>

class Cube : public Entity
{
public:
    Cube();
    ~Cube();

    void Render(Shader& aShader) override;

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    void SetupMesh();
};