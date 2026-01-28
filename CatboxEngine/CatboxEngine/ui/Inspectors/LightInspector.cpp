#include "LightInspector.h"
#include "imgui.h"
#include <iostream>

void LightInspector::Draw()
{
    auto& lightMgr = LightManager::Instance();
    
    ImGui::Begin("Lights");
    
    ImGui::Text("Active Lights: %zu", lightMgr.GetLightCount());
    ImGui::Separator();
    
    // Add new light button
    if (ImGui::Button("Add Directional Light"))
    {
        Light newLight;
        newLight.Name = "Directional Light";
        newLight.Type = LightType::Directional;
        newLight.Direction = {0, -1, 0};
        lightMgr.AddLight(newLight);
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Point Light"))
    {
        Light newLight;
        newLight.Name = "Point Light";
        newLight.Type = LightType::Point;
        newLight.Position = {0, 5, 0};
        lightMgr.AddLight(newLight);
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Spot Light"))
    {
        Light newLight;
        newLight.Name = "Spot Light";
        newLight.Type = LightType::Spot;
        newLight.Position = {0, 5, 0};
        newLight.Direction = {0, -1, 0};
        lightMgr.AddLight(newLight);
    }
    
    ImGui::Separator();
    
    // List all lights
    auto& lights = lightMgr.GetAllLights();
    for (size_t i = 0; i < lights.size(); ++i)
    {
        ImGui::PushID((int)i);
        
        auto& light = lights[i];
        
        // Collapsible header for each light
        bool open = ImGui::CollapsingHeader(
            (light.Name + " [" + GetLightTypeName(light.Type) + "]").c_str(),
            ImGuiTreeNodeFlags_DefaultOpen
        );
        
        if (open)
        {
            DrawLightProperties(light, i);
            
            ImGui::Separator();
        }
        
        ImGui::PopID();
    }
    
    ImGui::End();
}

void LightInspector::DrawLightProperties(Light& light, size_t index)
{
    auto& lightMgr = LightManager::Instance();
    
    // Name
    char nameBuf[128];
    strncpy_s(nameBuf, light.Name.c_str(), sizeof(nameBuf));
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
    {
        light.Name = nameBuf;
    }
    
    // Enabled checkbox
    ImGui::Checkbox("Enabled", &light.Enabled);
    
    // Light type
    const char* types[] = { "Directional", "Point", "Spot" };
    int currentType = (int)light.Type;
    if (ImGui::Combo("Type", &currentType, types, IM_ARRAYSIZE(types)))
    {
        light.Type = (LightType)currentType;
    }
    
    // Position (for Point and Spot lights)
    if (light.Type == LightType::Point || light.Type == LightType::Spot)
    {
        ImGui::DragFloat3("Position", &light.Position.x, 0.1f);
    }
    
    // Direction (for Directional and Spot lights)
    if (light.Type == LightType::Directional || light.Type == LightType::Spot)
    {
        if (ImGui::DragFloat3("Direction", &light.Direction.x, 0.01f))
        {
            // Normalize direction
            float length = std::sqrt(light.Direction.x * light.Direction.x + 
                                    light.Direction.y * light.Direction.y + 
                                    light.Direction.z * light.Direction.z);
            if (length > 0.001f)
            {
                light.Direction.x /= length;
                light.Direction.y /= length;
                light.Direction.z /= length;
            }
        }
    }
    
    // Color
    float color[3] = {light.Color.x, light.Color.y, light.Color.z};
    if (ImGui::ColorEdit3("Color", color))
    {
        light.Color = {color[0], color[1], color[2]};
    }
    
    // Intensity
    ImGui::SliderFloat("Intensity", &light.Intensity, 0.0f, 10.0f);
    
    // Attenuation (for Point and Spot lights)
    if (light.Type == LightType::Point || light.Type == LightType::Spot)
    {
        if (ImGui::TreeNode("Attenuation"))
        {
            ImGui::SliderFloat("Constant", &light.Constant, 0.0f, 10.0f);
            ImGui::SliderFloat("Linear", &light.Linear, 0.0f, 1.0f);
            ImGui::SliderFloat("Quadratic", &light.Quadratic, 0.0f, 1.0f);
            ImGui::TreePop();
        }
    }
    
    // Spot light cone (for Spot lights only)
    if (light.Type == LightType::Spot)
    {
        if (ImGui::TreeNode("Spotlight Cone"))
        {
            ImGui::SliderFloat("Inner Cutoff", &light.InnerCutoff, 0.0f, 90.0f);
            ImGui::SliderFloat("Outer Cutoff", &light.OuterCutoff, 0.0f, 90.0f);
            
            // Ensure outer > inner
            if (light.OuterCutoff < light.InnerCutoff)
                light.OuterCutoff = light.InnerCutoff;
            
            ImGui::TreePop();
        }
    }
    
    // Shadow properties
    if (ImGui::TreeNode("Shadows"))
    {
        bool castsShadows = light.CastsShadows;
        if (ImGui::Checkbox("Cast Shadows", &castsShadows))
        {
            light.CastsShadows = castsShadows;
            // TODO: Create/destroy shadow map
        }
        
        if (light.CastsShadows)
        {
            // Shadow map resolution
            const char* resolutions[] = { "512", "1024", "2048", "4096" };
            int currentRes = 1; // Default 1024
            if (light.ShadowMapSize == 512) currentRes = 0;
            else if (light.ShadowMapSize == 1024) currentRes = 1;
            else if (light.ShadowMapSize == 2048) currentRes = 2;
            else if (light.ShadowMapSize == 4096) currentRes = 3;
            
            if (ImGui::Combo("Resolution", &currentRes, resolutions, IM_ARRAYSIZE(resolutions)))
            {
                int sizes[] = {512, 1024, 2048, 4096};
                light.ShadowMapSize = sizes[currentRes];
                // TODO: Recreate shadow map with new size
            }
            
            ImGui::SliderFloat("Shadow Bias", &light.ShadowBias, 0.0001f, 0.01f, "%.4f");
            
            if (light.Type == LightType::Directional)
            {
                ImGui::SliderFloat("Ortho Size", &light.ShadowOrthoSize, 1.0f, 100.0f);
                ImGui::SliderFloat("Near Plane", &light.ShadowNearPlane, 0.1f, 10.0f);
                ImGui::SliderFloat("Far Plane", &light.ShadowFarPlane, 10.0f, 200.0f);
            }
            else
            {
                ImGui::SliderFloat("Shadow FOV", &light.ShadowFOV, 30.0f, 150.0f);
            }
        }
        
        ImGui::TreePop();
    }
    
    // Delete button
    ImGui::Spacing();
    if (ImGui::Button("Delete Light", ImVec2(120, 0)))
    {
        lightMgr.RemoveLight(index);
        ImGui::End();
        return;
    }
}

const char* LightInspector::GetLightTypeName(LightType type)
{
    switch (type)
    {
        case LightType::Directional: return "Dir";
        case LightType::Point: return "Point";
        case LightType::Spot: return "Spot";
        default: return "Unknown";
    }
}
