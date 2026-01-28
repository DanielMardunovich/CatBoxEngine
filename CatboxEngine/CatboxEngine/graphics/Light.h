#pragma once
#include "../resources/Math/Vec3.h"
#include <string>

enum class LightType
{
    Directional,
    Point,
    Spot
};

struct Light
{
    std::string Name = "Light";
    LightType Type = LightType::Directional;
    
    // Common properties
    Vec3 Position{0, 5, 0};          // For Point and Spot lights
    Vec3 Direction{0, -1, 0};        // For Directional and Spot lights
    Vec3 Color{1, 1, 1};             // RGB color
    float Intensity = 1.0f;
    
    // Point and Spot light attenuation
    float Constant = 1.0f;
    float Linear = 0.09f;
    float Quadratic = 0.032f;
    
    // Spot light properties
    float InnerCutoff = 12.5f;       // Degrees
    float OuterCutoff = 17.5f;       // Degrees
    
    // Shadow properties
    bool CastsShadows = true;
    int ShadowMapSize = 1024;
    unsigned int ShadowMapFBO = 0;
    unsigned int ShadowMapTexture = 0;
    float ShadowBias = 0.005f;
    
    // For directional lights (orthographic projection)
    float ShadowOrthoSize = 20.0f;
    float ShadowNearPlane = 1.0f;
    float ShadowFarPlane = 50.0f;
    
    // For point/spot lights (perspective projection)
    float ShadowFOV = 90.0f;
    
    bool Enabled = true;
};
