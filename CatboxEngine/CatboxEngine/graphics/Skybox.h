#pragma once
#include "Shader.h"
#include "Mesh.h"
#include <glm/glm.hpp>
#include <string>

// Renders a background sky behind all scene geometry.
//
// Procedural mode  (default): blends three configurable colours based on the
//   vertical component of the view direction — top, horizon, and bottom.
//
// Mesh mode: renders a skybox mesh loaded from a file (GLTF, GLB, OBJ)
//   with its own texture.  Call LoadFromFile() to switch to this mode.
class Skybox
{
public:
    Skybox();
    ~Skybox();

    Skybox(const Skybox&)            = delete;
    Skybox& operator=(const Skybox&) = delete;

    // Upload unit-cube geometry and compile the skybox shader.
    // Must be called after a valid OpenGL context exists.
    bool Initialize();

    // Load a skybox mesh from a file (OBJ, GLTF, GLB).
    // On success sets UseProceduralSky = false so the mesh is rendered.
    // On failure the previous mode is preserved and false is returned.
    bool LoadFromFile(const std::string& path);

    // Draw the skybox. Call AFTER all opaque geometry so the depth buffer is
    // already populated and the sky only fills pixels at maximum depth.
    // The view matrix should be rotation-only (no translation component).
    void Draw(const glm::mat4& view, const glm::mat4& projection);

    // ------- Procedural sky colours -------
    glm::vec3 SkyColorTop    { 0.10f, 0.40f, 0.80f };
    glm::vec3 SkyColorHorizon{ 0.50f, 0.70f, 0.90f };
    glm::vec3 SkyColorBottom { 0.30f, 0.25f, 0.15f };
    bool      UseProceduralSky = true;

private:
    // Unit cube used for procedural sky
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    Shader       m_shader;

    // Mesh-based skybox
    Mesh         m_mesh;
    bool         m_meshLoaded = false;

    void CleanupMesh();
};
