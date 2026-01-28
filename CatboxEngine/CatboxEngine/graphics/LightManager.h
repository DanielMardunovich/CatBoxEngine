#pragma once
#include "Light.h"
#include <vector>
#include <memory>

class LightManager
{
public:
    static LightManager& Instance()
    {
        static LightManager instance;
        return instance;
    }
    
    // Light management
    size_t AddLight(const Light& light);
    void RemoveLight(size_t index);
    Light* GetLight(size_t index);
    const Light* GetLight(size_t index) const;
    
    std::vector<Light>& GetAllLights() { return m_lights; }
    const std::vector<Light>& GetAllLights() const { return m_lights; }
    
    size_t GetLightCount() const { return m_lights.size(); }
    
    // Shadow map management
    void InitializeShadowMaps();
    void CleanupShadowMaps();
    void UpdateShadowMap(size_t lightIndex, class Camera& camera);
    
    // Create default lights
    void CreateDefaultLights();

private:
    LightManager() = default;
    ~LightManager();
    LightManager(const LightManager&) = delete;
    LightManager& operator=(const LightManager&) = delete;
    
    void CreateShadowMap(Light& light);
    void DeleteShadowMap(Light& light);
    
    std::vector<Light> m_lights;
};
