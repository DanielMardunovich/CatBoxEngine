#pragma once
#include "../../graphics/Light.h"
#include "../../graphics/LightManager.h"
#include "../../resources/Math/Vec3.h"

class LightInspector
{
public:
    LightInspector() = default;
    ~LightInspector() = default;

    // Disable copy, enable move
    LightInspector(const LightInspector&) = delete;
    LightInspector& operator=(const LightInspector&) = delete;
    LightInspector(LightInspector&&) noexcept = default;
    LightInspector& operator=(LightInspector&&) noexcept = default;

    void Draw();
    
private:
    void DrawSpawnControls();
    void DrawLightList();
    void DrawLightProperties();
    const char* GetLightTypeIcon(LightType type);
    
    // State
    int m_selectedLightIndex = -1;
    Vec3 m_lightSpawnPos = {0, 5, 0};
};
