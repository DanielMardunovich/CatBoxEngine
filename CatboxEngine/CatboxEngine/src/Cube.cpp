#include "Cube.h"
#include <array>
#include <glm/gtc/matrix_transform.hpp>

Cube::Cube()
{
    m_Model = glm::mat4(1.0f);
    SetupMesh();
}

Cube::Cube(const glm::vec3& position)
{
    m_Model = glm::translate(glm::mat4(1.0f), position);
    SetupMesh();
}

Cube::~Cube()
{
    if (EBO) glDeleteBuffers(1, &EBO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (VAO) glDeleteVertexArrays(1, &VAO);
}

void Cube::SetupMesh()
{
    // 8 vertices, each with position (x,y,z) and color (r,g,b)
    float vertices[] = {
        // positions          // colors
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f, // 0
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f, // 1
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f, // 2
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f, // 3
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f, // 4
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f, // 5
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 1.0f, // 6
        -0.5f,  0.5f,  0.5f,  0.3f, 0.7f, 0.2f  // 7
    };

    unsigned int indices[] = {
        // back face
        0,1,2, 2,3,0,
        // front face
        4,5,6, 6,7,4,
        // left face
        4,0,3, 3,7,4,
        // right face
        1,5,6, 6,2,1,
        // bottom face
        4,5,1, 1,0,4,
        // top face
        3,2,6, 6,7,3
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    // color attribute (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void Cube::SetModel(const glm::vec3& position, const glm::vec3& rotationDeg)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);

    // apply rotation in Z, Y, X order (adjust to your convention)
    model = glm::rotate(model, glm::radians(rotationDeg.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(rotationDeg.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotationDeg.x), glm::vec3(1.0f, 0.0f, 0.0f));

    m_Model = model;
}

void Cube::Render(Shader& aShader)
{
    // Upload this cube's model matrix before drawing so each cube can be placed independently.
    aShader.SetMat4("model", m_Model);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}