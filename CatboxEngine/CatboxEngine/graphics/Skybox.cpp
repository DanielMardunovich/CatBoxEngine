#include "Skybox.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>

// Unit cube positions — 36 vertices, position-only.
// Used as geometry AND as the procedural-sky sample direction.
static const float k_skyboxVerts[] =
{
    -1.0f,  1.0f, -1.0f,    -1.0f, -1.0f, -1.0f,     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,     1.0f,  1.0f, -1.0f,    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,    -1.0f, -1.0f, -1.0f,    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,    -1.0f,  1.0f,  1.0f,    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,     1.0f, -1.0f,  1.0f,     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,     1.0f,  1.0f, -1.0f,     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,    -1.0f,  1.0f,  1.0f,     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,     1.0f, -1.0f,  1.0f,    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,     1.0f,  1.0f, -1.0f,     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,    -1.0f,  1.0f,  1.0f,    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,    -1.0f, -1.0f,  1.0f,     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,    -1.0f, -1.0f,  1.0f,     1.0f, -1.0f,  1.0f
};

Skybox::Skybox()  = default;
Skybox::~Skybox()
{
    if (m_VAO != 0) { glDeleteVertexArrays(1, &m_VAO); m_VAO = 0; }
    if (m_VBO != 0) { glDeleteBuffers(1,      &m_VBO); m_VBO = 0; }
    CleanupMesh();
}

void Skybox::CleanupMesh()
{
    if (!m_meshLoaded) return;

    if (m_mesh.VAO != 0) { glDeleteVertexArrays(1, &m_mesh.VAO); m_mesh.VAO = 0; }
    if (m_mesh.VBO != 0) { glDeleteBuffers(1,      &m_mesh.VBO); m_mesh.VBO = 0; }
    if (m_mesh.EBO != 0) { glDeleteBuffers(1,      &m_mesh.EBO); m_mesh.EBO = 0; }

    for (auto& sub : m_mesh.SubMeshes)
    {
        if (sub.EBO != 0) { glDeleteBuffers(1, &sub.EBO); sub.EBO = 0; }
        if (sub.DiffuseTexture  != 0) { glDeleteTextures(1, &sub.DiffuseTexture);  sub.DiffuseTexture  = 0; }
        if (sub.SpecularTexture != 0) { glDeleteTextures(1, &sub.SpecularTexture); sub.SpecularTexture = 0; }
        if (sub.NormalTexture   != 0) { glDeleteTextures(1, &sub.NormalTexture);   sub.NormalTexture   = 0; }
    }

    if (m_mesh.DiffuseTexture  != 0) { glDeleteTextures(1, &m_mesh.DiffuseTexture);  m_mesh.DiffuseTexture  = 0; }
    if (m_mesh.SpecularTexture != 0) { glDeleteTextures(1, &m_mesh.SpecularTexture); m_mesh.SpecularTexture = 0; }

    m_mesh = Mesh{};
    m_meshLoaded = false;
}

bool Skybox::Initialize()
{
    m_shader.Initialize("./shaders/Skybox.vert", "./shaders/Skybox.frag");
    if (m_shader.GetProgram() == 0)
    {
        std::cerr << "Skybox: failed to compile shaders.\n";
        return false;
    }

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(k_skyboxVerts), k_skyboxVerts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    glBindVertexArray(0);
    return true;
}

bool Skybox::LoadFromFile(const std::string& path)
{
    // Determine file type from extension
    std::string ext = path;
    auto dot = ext.rfind('.');
    if (dot != std::string::npos)
        ext = ext.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    Mesh newMesh;
    bool ok = false;

    if (ext == ".gltf" || ext == ".glb")
        ok = newMesh.LoadFromGLTF(path);
    else if (ext == ".obj")
        ok = newMesh.LoadFromOBJ(path);
    else
    {
        std::cerr << "Skybox: unsupported file format \"" << ext << "\"\n";
        return false;
    }

    if (!ok)
    {
        std::cerr << "Skybox: failed to load mesh \"" << path << "\"\n";
        return false;
    }

    newMesh.Upload();

    if (newMesh.VAO == 0)
    {
        std::cerr << "Skybox: mesh upload failed for \"" << path << "\"\n";
        return false;
    }

    // Release any previously loaded skybox mesh
    CleanupMesh();

    m_mesh       = std::move(newMesh);
    m_meshLoaded = true;
    UseProceduralSky = false;

    std::cout << "Skybox: mesh loaded from \"" << path << "\" ("
              << m_mesh.Vertices.size() << " verts)\n";
    return true;
}

void Skybox::Draw(const glm::mat4& view, const glm::mat4& projection)
{
    if (m_shader.GetProgram() == 0)
        return;

    // Render at max depth so the sky fills only pixels untouched by geometry.
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    m_shader.Use();

    // Remove translation so the skybox stays centered on the camera.
    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));
    m_shader.SetMat4("u_View",       viewNoTranslation);
    m_shader.SetMat4("u_Projection", projection);
    m_shader.SetBool("u_UseProceduralSky", UseProceduralSky);
    m_shader.SetVec3("u_SkyColorTop",     SkyColorTop.x,     SkyColorTop.y,     SkyColorTop.z);
    m_shader.SetVec3("u_SkyColorHorizon", SkyColorHorizon.x, SkyColorHorizon.y, SkyColorHorizon.z);
    m_shader.SetVec3("u_SkyColorBottom",  SkyColorBottom.x,  SkyColorBottom.y,  SkyColorBottom.z);

    if (UseProceduralSky)
    {
        // Draw unit cube for procedural gradient
        if (m_VAO != 0)
        {
            glBindVertexArray(m_VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
        }
    }
    else if (m_meshLoaded && m_mesh.VAO != 0)
    {
        // Bind the first available diffuse texture from the mesh
        glActiveTexture(GL_TEXTURE0);
        unsigned int tex = 0;
        if (!m_mesh.SubMeshes.empty())
        {
            for (const auto& sub : m_mesh.SubMeshes)
            {
                if (sub.HasDiffuseTexture && sub.DiffuseTexture != 0)
                { tex = sub.DiffuseTexture; break; }
            }
        }
        if (tex == 0 && m_mesh.HasDiffuseTexture)
            tex = m_mesh.DiffuseTexture;

        if (tex != 0)
        {
            glBindTexture(GL_TEXTURE_2D, tex);
            m_shader.SetTexture("u_SkyTexture", 0);
        }

        m_mesh.Draw();
    }

    // Restore default state
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}
