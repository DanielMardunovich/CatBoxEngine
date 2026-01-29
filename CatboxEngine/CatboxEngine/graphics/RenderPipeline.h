#pragma once
#include "Shader.h"
#include "../resources/EntityManager.h"
#include "../resources/Camera.h"
#include "LightManager.h"
#include <glm/glm.hpp>

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
    void LightingPass();
    void PostProcessPass();
    
    // Debug rendering
    void RenderLightIndicators(const glm::mat4& viewProj);
    void RenderDebugInfo();
    
    // Settings
    void SetEnableShadows(bool enable) { m_enableShadows = enable; }
    void SetEnableFrustumCulling(bool enable) { m_enableFrustumCulling = enable; }
    void SetEnableLightIndicators(bool enable) { m_enableLightIndicators = enable; }
    
    bool GetEnableShadows() const { return m_enableShadows; }
    bool GetEnableFrustumCulling() const { return m_enableFrustumCulling; }
    bool GetEnableLightIndicators() const { return m_enableLightIndicators; }
    
    // Get rendering statistics
    const RenderStats& GetStats() const { return m_stats; }

private:
    // Shaders
    Shader m_mainShader;
    Shader m_shadowShader;
    
    // Settings
    bool m_enableShadows = true;
    bool m_enableFrustumCulling = true;
    bool m_enableLightIndicators = true;
    
    // Stats
    RenderStats m_stats;
    
    // Helper functions
    void SetupLightUniforms(const glm::mat4& viewProj);
    bool FrustumCullEntity(const class Entity& entity, class Mesh* mesh, const glm::mat4& model, Camera& camera);
};
