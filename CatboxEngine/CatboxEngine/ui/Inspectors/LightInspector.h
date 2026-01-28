#pragma once
#include "../../graphics/Light.h"
#include "../../graphics/LightManager.h"

class LightInspector
{
public:
    void Draw();
    
private:
    void DrawLightProperties(Light& light, size_t index);
    const char* GetLightTypeName(LightType type);
};
