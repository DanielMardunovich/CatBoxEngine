#pragma once
#include "Shader.h"
#include "Skybox.h"
#include "../resources/EntityManager.h"
#include "../resources/Camera.h"
#include "LightManager.h"
#include <glm/glm.hpp>
#include <vector>

// Render statistics for debugging
struct RenderStats
{
    int EntitiesRendered = 0;
    int EntitiesCulled = 0;
    int DrawCalls = 0;
    float ShadowPassTime = 0.0f;
    float MainPassTime = 0.0f;
    float LightPassTime = 0.0f;
    
    void Reset()
    {
        EntitiesRendered = 0;
        EntitiesCulled = 0;
        DrawCalls = 0;
        ShadowPassTime = 0.0f;
        MainPassTime = 0.0f;
        LightPassTime = 0.0f;
    }
};

// Rendering pipeline handles all rendering stages
class RenderPipeline
{
public:
    RenderPipeline();
    ~RenderPipeline();
    
    // Initialize shaders and resources
    bool Initialize();
    
    // Main render call - executes full pipeline
    void Render(EntityManager& entityManager, Camera& camera, int displayWidth, int displayHeight);
    
    // Individual render passes
    void ShadowPass(EntityManager& entityManager);
    void GeometryPass(EntityManager& entityManager, Camera& camera, const glm::mat4& viewProj);
    
    // Debug rendering
    void RenderLightIndicators(const glm::mat4& viewProj);

    // Editor-only overlay: patrol waypoint nodes and connecting lines
    // Call this only when NOT in play mode.
    void RenderWaypointOverlay(EntityManager& entityManager, Camera& camera, int displayWidth, int displayHeight);
    
    // Settings
    void SetEnableShadows(bool enable) { m_enableShadows = enable; }
    void SetEnableFrustumCulling(bool enable) { m_enableFrustumCulling = enable; }
    void SetEnableLightIndicators(bool enable) { m_enableLightIndicators = enable; }

    bool GetEnableShadows() const { return m_enableShadows; }
    bool GetEnableFrustumCulling() const { return m_enableFrustumCulling; }
    bool GetEnableLightIndicators() const { return m_enableLightIndicators; }

    // Skybox access (colours / mode are edited via GraphicsSettings)
    Skybox& GetSkybox() { return m_skybox; }
    
    // Get rendering statistics
    const RenderStats& GetStats() const { return m_stats; }

private:
    // Shaders
    Shader m_mainShader;
    Shader m_shadowShader;

    // Skybox
    Skybox m_skybox;

    // Settings
    bool m_enableShadows = true;
    bool m_enableFrustumCulling = true;
    bool m_enableLightIndicators = true;
    
    // Stats
    RenderStats m_stats;

    // Line renderer for debug overlays
    unsigned int m_lineVAO = 0;
    unsigned int m_lineVBO = 0;
    static constexpr int MAX_LINE_VERTS = 2048;
    void InitLineRenderer();
    void DrawLines(const std::vector<glm::vec3>& linePoints, const glm::mat4& viewProj, float r, float g, float b);

    // Helper functions
    void SetupLightUniforms(const glm::mat4& viewProj);
    bool FrustumCullEntity(const class Entity& entity, class Mesh* mesh, const glm::mat4& model, Camera& camera);
};
