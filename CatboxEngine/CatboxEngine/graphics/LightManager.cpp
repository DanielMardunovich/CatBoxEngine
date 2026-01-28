#include "LightManager.h"
#include <glad/glad.h>
#include <iostream>

LightManager::~LightManager()
{
    CleanupShadowMaps();
}

size_t LightManager::AddLight(const Light& light)
{
    m_lights.push_back(light);
    size_t index = m_lights.size() - 1;
    
    // Create shadow map if needed
    if (m_lights[index].CastsShadows)
    {
        CreateShadowMap(m_lights[index]);
    }
    
    std::cout << "Light added: " << light.Name << " (Index: " << index << ")" << std::endl;
    return index;
}

void LightManager::RemoveLight(size_t index)
{
    if (index >= m_lights.size())
        return;
    
    DeleteShadowMap(m_lights[index]);
    m_lights.erase(m_lights.begin() + index);
    
    std::cout << "Light removed at index: " << index << std::endl;
}

Light* LightManager::GetLight(size_t index)
{
    if (index >= m_lights.size())
        return nullptr;
    return &m_lights[index];
}

const Light* LightManager::GetLight(size_t index) const
{
    if (index >= m_lights.size())
        return nullptr;
    return &m_lights[index];
}

void LightManager::CreateShadowMap(Light& light)
{
    // Create framebuffer
    glGenFramebuffers(1, &light.ShadowMapFBO);
    
    // Create depth texture
    glGenTextures(1, &light.ShadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, light.ShadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                 light.ShadowMapSize, light.ShadowMapSize, 0, 
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    // Attach depth texture to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, light.ShadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
                           GL_TEXTURE_2D, light.ShadowMapTexture, 0);
    
    // No color buffer needed
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    // Check framebuffer status
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Shadow map framebuffer incomplete!" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    std::cout << "Shadow map created: " << light.ShadowMapSize << "x" << light.ShadowMapSize << std::endl;
}

void LightManager::DeleteShadowMap(Light& light)
{
    if (light.ShadowMapFBO != 0)
    {
        glDeleteFramebuffers(1, &light.ShadowMapFBO);
        light.ShadowMapFBO = 0;
    }
    
    if (light.ShadowMapTexture != 0)
    {
        glDeleteTextures(1, &light.ShadowMapTexture);
        light.ShadowMapTexture = 0;
    }
}

void LightManager::InitializeShadowMaps()
{
    for (auto& light : m_lights)
    {
        if (light.CastsShadows && light.ShadowMapFBO == 0)
        {
            CreateShadowMap(light);
        }
    }
}

void LightManager::CleanupShadowMaps()
{
    for (auto& light : m_lights)
    {
        DeleteShadowMap(light);
    }
}

void LightManager::CreateDefaultLights()
{
    // Add a default directional light (sun)
    Light sunLight;
    sunLight.Name = "Sun";
    sunLight.Type = LightType::Directional;
    sunLight.Direction = {0.5f, -0.7f, 0.3f};
    sunLight.Color = {1.0f, 0.95f, 0.8f};  // Warm sunlight
    sunLight.Intensity = 1.0f;
    sunLight.CastsShadows = true;
    AddLight(sunLight);
    
    // Add a point light
    Light pointLight;
    pointLight.Name = "Point Light";
    pointLight.Type = LightType::Point;
    pointLight.Position = {0, 5, 0};
    pointLight.Color = {1.0f, 1.0f, 1.0f};
    pointLight.Intensity = 1.0f;
    pointLight.CastsShadows = false;
    AddLight(pointLight);
    
    std::cout << "Default lights created" << std::endl;
}
