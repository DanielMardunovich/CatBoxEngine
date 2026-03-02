#include "RenderPipeline.h"
#include "Mesh.h"
#include "MeshManager.h"
#include "Light.h"
#include "../resources/Entity.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cfloat>

RenderPipeline::RenderPipeline()
{
}

RenderPipeline::~RenderPipeline()
{
    if (m_lineVAO != 0) { glDeleteVertexArrays(1, &m_lineVAO); m_lineVAO = 0; }
    if (m_lineVBO != 0) { glDeleteBuffers(1, &m_lineVBO); m_lineVBO = 0; }
}

bool RenderPipeline::Initialize()
{
    // Initialize main shader
    m_mainShader.Initialize("./shaders/VertexShader.vert", "./shaders/FragmentShader.frag");
    
    // Initialize shadow shader
    m_shadowShader.Initialize("./shaders/ShadowMap.vert", "./shaders/ShadowMap.frag");

    InitLineRenderer();

    return true;
}

void RenderPipeline::Render(EntityManager& entityManager, Camera& camera, int displayWidth, int displayHeight)
{
    m_stats.Reset();
    
    // 1. Shadow Pass - Render shadow maps for all lights
    if (m_enableShadows)
    {
        ShadowPass(entityManager);
    }
    
    // Reset viewport for main rendering
    glViewport(0, 0, displayWidth, displayHeight);
    
    // 2. Geometry Pass - Render scene with lighting
    camera.Aspect = (float)displayWidth / (float)displayHeight;
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix();
    glm::mat4 viewProj = proj * view;
    
    GeometryPass(entityManager, camera, viewProj);
    
    // 3. Render light indicators (debug visualization)
    if (m_enableLightIndicators)
    {
        RenderLightIndicators(viewProj);
    }
}

void RenderPipeline::ShadowPass(EntityManager& entityManager)
{
    auto& lightMgr = LightManager::Instance();
    auto& lights = lightMgr.GetAllLights();
    
    m_shadowShader.Use();
    
    // Store current framebuffer to restore later
    GLint currentFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    
    // Disable color writes, only depth
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    
    // Enable depth test for shadow rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    for (size_t lightIdx = 0; lightIdx < lights.size(); ++lightIdx)
    {
        auto& light = lights[lightIdx];
        
        if (!light.CastsShadows || !light.Enabled || light.ShadowMapFBO == 0)
            continue;
        
        // Bind shadow map framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, light.ShadowMapFBO);
        glViewport(0, 0, light.ShadowMapSize, light.ShadowMapSize);
        glClear(GL_DEPTH_BUFFER_BIT);
        
        // Check framebuffer status
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Shadow framebuffer incomplete for light: " << light.Name << std::endl;
            continue;
        }
        
        glm::mat4 lightProjection, lightView, lightSpaceMatrix;
        
        if (light.Type == LightType::Directional)
        {
            float size = light.ShadowOrthoSize;
            lightProjection = glm::ortho(-size, size, -size, size, light.ShadowNearPlane, light.ShadowFarPlane);
            
            // Position light opposite to direction
            glm::vec3 lightPos(-light.Direction.x * 10.0f, -light.Direction.y * 10.0f, -light.Direction.z * 10.0f);
            glm::vec3 target = lightPos + glm::vec3(light.Direction.x, light.Direction.y, light.Direction.z);
            lightView = glm::lookAt(lightPos, target, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else // Point or Spot light
        {
            // Normalize direction
            glm::vec3 direction(light.Direction.x, light.Direction.y, light.Direction.z);
            float dirLength = glm::length(direction);
            if (dirLength > 0.001f)
            {
                direction = glm::normalize(direction);
            }
            else
            {
                direction = glm::vec3(0.0f, -1.0f, 0.0f); // Default downward
            }
            
            float fov = (light.Type == LightType::Spot) ? light.OuterCutoff * 2.0f : light.ShadowFOV;
            lightProjection = glm::perspective(glm::radians(fov), 1.0f, light.ShadowNearPlane, light.ShadowFarPlane);
            
            glm::vec3 lightPos(light.Position.x, light.Position.y, light.Position.z);
            glm::vec3 target = lightPos + direction;
            
            // Choose appropriate up vector
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            if (std::abs(direction.y) > 0.99f)  // Near vertical
            {
                up = glm::vec3(1.0f, 0.0f, 0.0f);
            }
            
            lightView = glm::lookAt(lightPos, target, up);
        }
        
        lightSpaceMatrix = lightProjection * lightView;
        m_shadowShader.SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);
        light.LightSpaceMatrix = lightSpaceMatrix;
        
        // Render entities to shadow map
        for (const auto& e : entityManager.GetAll())
        {
            Mesh* mesh = e.MeshHandle ? MeshManager::Instance().GetMesh(e.MeshHandle) : nullptr;
            if (!mesh || mesh->VAO == 0) continue;
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(e.Transform.Position.x, e.Transform.Position.y, e.Transform.Position.z));
            model = glm::rotate(model, glm::radians(e.Transform.Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(e.Transform.Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(e.Transform.Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(e.Transform.Scale.x, e.Transform.Scale.y, e.Transform.Scale.z));
            
            m_shadowShader.SetMat4("u_Model", model);
            mesh->Draw();
        }
    }
    
    // CRITICAL: Restore default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
    
    // Re-enable color writes
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void RenderPipeline::GeometryPass(EntityManager& entityManager, Camera& camera, const glm::mat4& viewProj)
{
    m_mainShader.Use();
    m_mainShader.SetBool("u_IsUnlit", false);  // safety: clear any leftover overlay state
    m_mainShader.SetVec3("u_CameraPos", camera.Position.x, camera.Position.y, camera.Position.z);
    SetupLightUniforms(viewProj);
    
    for (const auto& e : entityManager.GetAll())
    {
        Mesh* mesh = e.MeshHandle ? MeshManager::Instance().GetMesh(e.MeshHandle) : nullptr;
        if (!mesh) continue;
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(e.Transform.Position.x, e.Transform.Position.y, e.Transform.Position.z));
        model = glm::rotate(model, glm::radians(e.Transform.Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(e.Transform.Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(e.Transform.Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(e.Transform.Scale.x, e.Transform.Scale.y, e.Transform.Scale.z));
        
        if (m_enableFrustumCulling && FrustumCullEntity(e, mesh, model, camera))
        {
            m_stats.EntitiesCulled++;
            continue;
        }
        
        m_stats.EntitiesRendered++;
        m_mainShader.SetMat4("u_MVP", viewProj);
        m_mainShader.SetMat4("transform", model);
        
        if (!mesh->SubMeshes.empty())
        {
            glBindVertexArray(mesh->VAO);
            for (const auto& sub : mesh->SubMeshes)
            {
                m_mainShader.SetVec3("u_DiffuseColor", sub.DiffuseColor.x, sub.DiffuseColor.y, sub.DiffuseColor.z);
                
                // Check for entity texture override first, then submesh texture
                bool hasDiffuse = e.HasDiffuseTextureOverride || sub.HasDiffuseTexture;
                m_mainShader.SetBool("u_HasDiffuseMap", hasDiffuse);
                if (hasDiffuse)
                {
                    glActiveTexture(GL_TEXTURE0);
                    // Use entity override if available, otherwise use submesh texture
                    unsigned int texToUse = e.HasDiffuseTextureOverride ? e.DiffuseTexture : sub.DiffuseTexture;
                    glBindTexture(GL_TEXTURE_2D, texToUse);
                    m_mainShader.SetTexture("u_DiffuseMap", 0);
                }
                
                // Check for normal map override
                bool hasNormal = e.HasNormalTextureOverride || sub.HasNormalTexture;
                m_mainShader.SetBool("u_HasNormalMap", hasNormal);
                if (hasNormal)
                {
                    glActiveTexture(GL_TEXTURE2);
                    unsigned int texToUse = e.HasNormalTextureOverride ? e.NormalTexture : sub.NormalTexture;
                    glBindTexture(GL_TEXTURE_2D, texToUse);
                    m_mainShader.SetTexture("u_NormalMap", 2);
                }
                
                // Check for specular map override
                bool hasSpecular = e.HasSpecularTextureOverride || sub.HasSpecularTexture;
                m_mainShader.SetBool("u_HasSpecularMap", hasSpecular);
                if (hasSpecular)
                {
                    glActiveTexture(GL_TEXTURE1);
                    unsigned int texToUse = e.HasSpecularTextureOverride ? e.SpecularTexture : sub.SpecularTexture;
                    glBindTexture(GL_TEXTURE_2D, texToUse);
                    m_mainShader.SetTexture("u_SpecularMap", 1);
                }
                
                m_mainShader.SetVec3("u_SpecularColor", sub.SpecularColor.x, sub.SpecularColor.y, sub.SpecularColor.z);
                m_mainShader.SetFloat("u_Shininess", e.Shininess);
                m_mainShader.SetFloat("u_Alpha", e.Alpha);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sub.EBO);
                glDrawElements(GL_TRIANGLES, (GLsizei)sub.Indices.size(), GL_UNSIGNED_INT, 0);
                m_stats.DrawCalls++;
            }
        }
        else
        {
            m_mainShader.SetVec3("u_DiffuseColor", mesh->DiffuseColor.x, mesh->DiffuseColor.y, mesh->DiffuseColor.z);
            
            // Check for entity texture overrides
            bool hasDiffuse = e.HasDiffuseTextureOverride || mesh->HasDiffuseTexture;
            m_mainShader.SetBool("u_HasDiffuseMap", hasDiffuse);
            if (hasDiffuse)
            {
                glActiveTexture(GL_TEXTURE0);
                unsigned int texToUse = e.HasDiffuseTextureOverride ? e.DiffuseTexture : mesh->DiffuseTexture;
                glBindTexture(GL_TEXTURE_2D, texToUse);
                m_mainShader.SetTexture("u_DiffuseMap", 0);
            }
            
            bool hasNormal = e.HasNormalTextureOverride || mesh->HasNormalTexture;
            m_mainShader.SetBool("u_HasNormalMap", hasNormal);
            if (hasNormal)
            {
                glActiveTexture(GL_TEXTURE2);
                unsigned int texToUse = e.HasNormalTextureOverride ? e.NormalTexture : mesh->NormalTexture;
                glBindTexture(GL_TEXTURE_2D, texToUse);
                m_mainShader.SetTexture("u_NormalMap", 2);
            }
            
            bool hasSpecular = e.HasSpecularTextureOverride || mesh->HasSpecularTexture;
            m_mainShader.SetBool("u_HasSpecularMap", hasSpecular);
            if (hasSpecular)
            {
                glActiveTexture(GL_TEXTURE1);
                unsigned int texToUse = e.HasSpecularTextureOverride ? e.SpecularTexture : mesh->SpecularTexture;
                glBindTexture(GL_TEXTURE_2D, texToUse);
                m_mainShader.SetTexture("u_SpecularMap", 1);
            }
            
            m_mainShader.SetVec3("u_SpecularColor", mesh->SpecularColor.x, mesh->SpecularColor.y, mesh->SpecularColor.z);
            m_mainShader.SetFloat("u_Shininess", e.Shininess);
            m_mainShader.SetFloat("u_Alpha", e.Alpha);
            if (mesh->VAO != 0)
            {
                mesh->Draw();
                m_stats.DrawCalls++;
            }
        }
    }
}

void RenderPipeline::SetupLightUniforms(const glm::mat4& viewProj)
{
    auto& lights = LightManager::Instance().GetAllLights();
    int numLights = std::min((int)lights.size(), 8);
    m_mainShader.SetInt("u_NumLights", numLights);
    
    for (int i = 0; i < numLights; ++i)
    {
        const auto& light = lights[i];
        std::string base = "u_Lights[" + std::to_string(i) + "].";
        m_mainShader.SetInt((base + "type").c_str(), (int)light.Type);
        m_mainShader.SetVec3((base + "position").c_str(), light.Position.x, light.Position.y, light.Position.z);
        m_mainShader.SetVec3((base + "direction").c_str(), light.Direction.x, light.Direction.y, light.Direction.z);
        m_mainShader.SetVec3((base + "color").c_str(), light.Color.x, light.Color.y, light.Color.z);
        m_mainShader.SetFloat((base + "intensity").c_str(), light.Intensity);
        m_mainShader.SetFloat((base + "constant").c_str(), light.Constant);
        m_mainShader.SetFloat((base + "linear").c_str(), light.Linear);
        m_mainShader.SetFloat((base + "quadratic").c_str(), light.Quadratic);
        m_mainShader.SetFloat((base + "innerCutoff").c_str(), std::cos(glm::radians(light.InnerCutoff)));
        m_mainShader.SetFloat((base + "outerCutoff").c_str(), std::cos(glm::radians(light.OuterCutoff)));
        m_mainShader.SetBool((base + "castsShadows").c_str(), light.CastsShadows && light.Enabled);
        m_mainShader.SetFloat((base + "shadowBias").c_str(), light.ShadowBias);
        m_mainShader.SetBool((base + "enabled").c_str(), light.Enabled);
        m_mainShader.SetMat4(("u_LightSpaceMatrices[" + std::to_string(i) + "]").c_str(), light.LightSpaceMatrix);
        if (light.CastsShadows && light.Enabled)
        {
            glActiveTexture(GL_TEXTURE3 + i);
            glBindTexture(GL_TEXTURE_2D, light.ShadowMapTexture);
            m_mainShader.SetInt((base + "shadowMap").c_str(), 3 + i);
        }
    }
}

bool RenderPipeline::FrustumCullEntity(const Entity& entity, Mesh* mesh, const glm::mat4& model, Camera& camera)
{
    bool validBounds = (mesh->BoundsMin.x != FLT_MAX) && (mesh->BoundsMax.x != -FLT_MAX);
    if (!validBounds) return false;
    
    glm::vec4 worldMin = model * glm::vec4(mesh->BoundsMin.x, mesh->BoundsMin.y, mesh->BoundsMin.z, 1.0f);
    glm::vec4 worldMax = model * glm::vec4(mesh->BoundsMax.x, mesh->BoundsMax.y, mesh->BoundsMax.z, 1.0f);
    return !camera.IsBoxInFrustum(Vec3{worldMin.x, worldMin.y, worldMin.z}, Vec3{worldMax.x, worldMax.y, worldMax.z});
}

void RenderPipeline::RenderLightIndicators(const glm::mat4& viewProj)
{
    auto& lights = LightManager::Instance().GetAllLights();
    if (lights.empty()) return;
    
    static MeshHandle lightCubeMesh = 0;
    if (lightCubeMesh == 0) lightCubeMesh = MeshManager::Instance().GetSharedCubeHandle();
    
    Mesh* cubeMesh = MeshManager::Instance().GetMesh(lightCubeMesh);
    if (!cubeMesh) return;
    
    for (const auto& light : lights)
    {
        if (light.Type == LightType::Directional) continue;
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(light.Position.x, light.Position.y, light.Position.z));
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        
        m_mainShader.SetMat4("u_MVP", viewProj);
        m_mainShader.SetMat4("transform", model);
        m_mainShader.SetVec3("u_DiffuseColor", light.Color.x, light.Color.y, light.Color.z);
        m_mainShader.SetBool("u_HasDiffuseMap", false);
        m_mainShader.SetBool("u_HasSpecularMap", false);
        m_mainShader.SetBool("u_HasNormalMap", false);
        m_mainShader.SetFloat("u_Alpha", light.Enabled ? 1.0f : 0.3f);
        
        if (cubeMesh->VAO != 0) cubeMesh->Draw();
    }
}

void RenderPipeline::InitLineRenderer()
{
    glGenVertexArrays(1, &m_lineVAO);
    glGenBuffers(1, &m_lineVBO);

    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_LINE_VERTS * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

    // Only position attribute (location 0); normals/UVs/tangents default to 0, which is fine
    // for unlit rendering.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glBindVertexArray(0);
}

void RenderPipeline::DrawLines(const std::vector<glm::vec3>& linePoints,
                               const glm::mat4& viewProj, float r, float g, float b)
{
    if (linePoints.empty() || m_lineVAO == 0) return;

    const size_t count = std::min(linePoints.size(), static_cast<size_t>(MAX_LINE_VERTS));

    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    static_cast<GLsizeiptr>(count * sizeof(glm::vec3)), linePoints.data());

    m_mainShader.SetMat4("u_MVP", viewProj);
    m_mainShader.SetMat4("transform", glm::mat4(1.0f));
    m_mainShader.SetVec3("u_DiffuseColor", r, g, b);

    glLineWidth(2.0f);
    glBindVertexArray(m_lineVAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(count));
    glBindVertexArray(0);
    glLineWidth(1.0f);
}

void RenderPipeline::RenderWaypointOverlay(EntityManager& entityManager,
                                           Camera& camera, int displayWidth, int displayHeight)
{
    camera.Aspect = static_cast<float>(displayWidth) / static_cast<float>(displayHeight);
    const glm::mat4 viewProj = camera.GetProjectionMatrix() * camera.GetViewMatrix();

    static MeshHandle s_cubeMesh = 0;
    if (s_cubeMesh == 0)
        s_cubeMesh = MeshManager::Instance().GetSharedCubeHandle();
    Mesh* cube = MeshManager::Instance().GetMesh(s_cubeMesh);
    if (!cube || cube->VAO == 0) return;

    m_mainShader.Use();
    m_mainShader.SetBool("u_IsUnlit", true);
    m_mainShader.SetBool("u_HasDiffuseMap", false);
    m_mainShader.SetBool("u_HasSpecularMap", false);
    m_mainShader.SetBool("u_HasNormalMap", false);
    m_mainShader.SetFloat("u_Alpha", 1.0f);
    m_mainShader.SetMat4("u_MVP", viewProj);

    for (const auto& entity : entityManager.GetAll())
    {
        if (!entity.IsEnemy || entity.PatrolWaypoints.empty())
            continue;

        const size_t count = entity.PatrolWaypoints.size();

        // --- Waypoint nodes ---
        for (size_t w = 0; w < count; ++w)
        {
            const Vec3& wp = entity.PatrolWaypoints[w];
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(wp.x, wp.y, wp.z));
            model = glm::scale(model, glm::vec3(0.3f));
            m_mainShader.SetMat4("transform", model);

            // First waypoint (start) is green, rest are orange
            if (w == 0)
                m_mainShader.SetVec3("u_DiffuseColor", 0.1f, 1.0f, 0.1f);
            else
                m_mainShader.SetVec3("u_DiffuseColor", 1.0f, 0.5f, 0.0f);

            cube->Draw();
        }

        // --- Connecting lines ---
        std::vector<glm::vec3> lineVerts;
        lineVerts.reserve(count * 2);

        for (size_t w = 0; w + 1 < count; ++w)
        {
            lineVerts.emplace_back(entity.PatrolWaypoints[w].x,   entity.PatrolWaypoints[w].y,   entity.PatrolWaypoints[w].z);
            lineVerts.emplace_back(entity.PatrolWaypoints[w+1].x, entity.PatrolWaypoints[w+1].y, entity.PatrolWaypoints[w+1].z);
        }

        // Loop mode: also connect last waypoint back to first
        if (entity.EnemyPatrolMode == PatrolMode::Loop && count > 1)
        {
            lineVerts.emplace_back(entity.PatrolWaypoints[count - 1].x, entity.PatrolWaypoints[count - 1].y, entity.PatrolWaypoints[count - 1].z);
            lineVerts.emplace_back(entity.PatrolWaypoints[0].x,         entity.PatrolWaypoints[0].y,         entity.PatrolWaypoints[0].z);
        }

        if (!lineVerts.empty())
            DrawLines(lineVerts, viewProj, 1.0f, 1.0f, 1.0f);  // white
    }

    m_mainShader.SetBool("u_IsUnlit", false);
}

void RenderPipeline::LightingPass() {}
void RenderPipeline::PostProcessPass() {}
void RenderPipeline::RenderDebugInfo() {}
