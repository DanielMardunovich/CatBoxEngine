#include "LightInspector.h"
#include "imgui.h"
#include <glad/glad.h>
#include <iostream>
#include <cmath>

void LightInspector::Draw()
{
    auto& lightMgr = LightManager::Instance();
    
    ImGui::Begin("Lights");
    
    ImGui::Text("Active Lights: %zu", lightMgr.GetLightCount());
    ImGui::Separator();
    
    // Spawn controls
    DrawSpawnControls();
    
    ImGui::Separator();
    
    // Light list
    ImGui::Text("Light List:");
    DrawLightList();
    
    ImGui::Separator();
    
    // Selected light properties
    DrawLightProperties();
    
    ImGui::End();
}

void LightInspector::DrawSpawnControls()
{
    auto& lightMgr = LightManager::Instance();
    
    ImGui::Text("Spawn Position");
    ImGui::DragFloat3("##SpawnPos", &m_lightSpawnPos.x, 0.1f);
    
    // Spawn buttons in a row
    if (ImGui::Button("Spawn Point Light"))
    {
        Light newLight;
        newLight.Name = "Point Light " + std::to_string(lightMgr.GetLightCount());
        newLight.Type = LightType::Point;
        newLight.Position = m_lightSpawnPos;
        newLight.Color = {1, 1, 1};
        newLight.Intensity = 1.0f;
        newLight.CastsShadows = false;
        lightMgr.AddLight(newLight);
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Spawn Directional Light"))
    {
        Light newLight;
        newLight.Name = "Directional Light " + std::to_string(lightMgr.GetLightCount());
        newLight.Type = LightType::Directional;
        newLight.Direction = {0, -1, 0};
        newLight.Color = {1, 1, 1};
        newLight.Intensity = 1.0f;
        newLight.CastsShadows = true;
        lightMgr.AddLight(newLight);
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Spawn Spot Light"))
    {
        Light newLight;
        newLight.Name = "Spot Light " + std::to_string(lightMgr.GetLightCount());
        newLight.Type = LightType::Spot;
        newLight.Position = m_lightSpawnPos;
        newLight.Direction = {0, -1, 0};
        newLight.Color = {1, 1, 1};
        newLight.Intensity = 1.0f;
        newLight.CastsShadows = false;
        lightMgr.AddLight(newLight);
    }
}

void LightInspector::DrawLightList()
{
    auto& lightMgr = LightManager::Instance();
    auto& lights = lightMgr.GetAllLights();
    
    ImGui::BeginChild("LightListScroll", ImVec2(0, 200), true);
    ImGui::Columns(2);
    ImGui::SetColumnWidth(1, 90.0f);
    
    for (size_t i = 0; i < lights.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));
        
        auto& light = lights[i];
        bool isSelected = (m_selectedLightIndex == static_cast<int>(i));
        
        // Display name with icon and disabled status
        std::string displayName = std::string(GetLightTypeIcon(light.Type)) + " " + light.Name;
        if (!light.Enabled)
            displayName += " (Disabled)";
        
        // Left column: selectable name
        if (ImGui::Selectable(displayName.c_str(), isSelected))
        {
            m_selectedLightIndex = static_cast<int>(i);
        }
        
        ImGui::NextColumn();
        
        // Right column: delete button
        ImGui::AlignTextToFramePadding();
        if (ImGui::SmallButton("Delete"))
        {
            lightMgr.RemoveLight(i);
            if (m_selectedLightIndex == static_cast<int>(i))
            {
                m_selectedLightIndex = -1;
            }
            else if (m_selectedLightIndex > static_cast<int>(i))
            {
                m_selectedLightIndex -= 1;
            }
            ImGui::PopID();
            break;
        }
        
        ImGui::NextColumn();
        ImGui::PopID();
    }
    
    ImGui::Columns(1);
    ImGui::EndChild();
}

void LightInspector::DrawLightProperties()
{
    auto& lightMgr = LightManager::Instance();
    auto& lights = lightMgr.GetAllLights();
    
    if (m_selectedLightIndex < 0 || m_selectedLightIndex >= static_cast<int>(lights.size()))
    {
        return;
    }
    
    auto& light = lights[m_selectedLightIndex];
    
    ImGui::Text("Selected: %s", light.Name.c_str());
    ImGui::Separator();
    
    // Name
    char nameBuf[128];
    strncpy_s(nameBuf, light.Name.c_str(), sizeof(nameBuf));
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
    {
        light.Name = nameBuf;
    }
    
    // Enabled
    ImGui::Checkbox("Enabled", &light.Enabled);
    
    // Type
    const char* types[] = { "Directional", "Point", "Spot" };
    int currentType = static_cast<int>(light.Type);
    if (ImGui::Combo("Type", &currentType, types, 3))
    {
        light.Type = static_cast<LightType>(currentType);
    }
    
    // Position (for Point and Spot)
    if (light.Type == LightType::Point || light.Type == LightType::Spot)
    {
        ImGui::DragFloat3("Position", &light.Position.x, 0.1f);
    }
    
    // Direction (for Directional and Spot)
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
    
    // Attenuation (for Point and Spot)
    if (light.Type == LightType::Point || light.Type == LightType::Spot)
    {
        if (ImGui::TreeNode("Attenuation"))
        {
            ImGui::SliderFloat("Constant", &light.Constant, 0.0f, 10.0f);
            ImGui::SliderFloat("Linear", &light.Linear, 0.0f, 1.0f);
            ImGui::SliderFloat("Quadratic", &light.Quadratic, 0.0f, 1.0f);
            
            // Show effective range
            float range = (-light.Linear + std::sqrt(light.Linear * light.Linear - 
                          4.0f * light.Quadratic * (light.Constant - 256.0f * light.Intensity))) / 
                          (2.0f * light.Quadratic);
            ImGui::Text("Effective Range: %.1f units", range);
            
            ImGui::TreePop();
        }
    }
    
    // Spot cone
    if (light.Type == LightType::Spot)
    {
        if (ImGui::TreeNode("Spotlight Cone"))
        {
            ImGui::SliderFloat("Inner Cutoff", &light.InnerCutoff, 0.0f, 90.0f);
            ImGui::SliderFloat("Outer Cutoff", &light.OuterCutoff, 0.0f, 90.0f);
            
            if (light.OuterCutoff < light.InnerCutoff)
                light.OuterCutoff = light.InnerCutoff;
            
            ImGui::TreePop();
        }
    }
    
    // Shadows
    if (ImGui::TreeNode("Shadows"))
    {
        bool castsShadows = light.CastsShadows;
        if (ImGui::Checkbox("Cast Shadows", &castsShadows))
        {
            light.CastsShadows = castsShadows;
            
            if (castsShadows && light.ShadowMapFBO == 0)
            {
                // Create shadow map
                glGenFramebuffers(1, &light.ShadowMapFBO);
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
                
                glBindFramebuffer(GL_FRAMEBUFFER, light.ShadowMapFBO);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                      GL_TEXTURE_2D, light.ShadowMapTexture, 0);
                
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
                
                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                {
                    std::cerr << "ERROR: Shadow framebuffer incomplete!" << std::endl;
                }
                
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                
                std::cout << "Shadow map created for " << light.Name << std::endl;
            }
            else if (!castsShadows && light.ShadowMapFBO != 0)
            {
                glDeleteFramebuffers(1, &light.ShadowMapFBO);
                glDeleteTextures(1, &light.ShadowMapTexture);
                light.ShadowMapFBO = 0;
                light.ShadowMapTexture = 0;
                
                std::cout << "Shadow map destroyed for " << light.Name << std::endl;
            }
        }
        
        if (light.CastsShadows)
        {
            const char* resolutions[] = { "512", "1024", "2048", "4096" };
            int currentRes = 1;
            if (light.ShadowMapSize == 512) currentRes = 0;
            else if (light.ShadowMapSize == 1024) currentRes = 1;
            else if (light.ShadowMapSize == 2048) currentRes = 2;
            else if (light.ShadowMapSize == 4096) currentRes = 3;
            
            if (ImGui::Combo("Resolution", &currentRes, resolutions, 4))
            {
                int sizes[] = {512, 1024, 2048, 4096};
                light.ShadowMapSize = sizes[currentRes];
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
                ImGui::SliderFloat("Shadow FOV", &light.ShadowFOV, 60.0f, 179.0f, "%.0f°");
                ImGui::SliderFloat("Near Plane", &light.ShadowNearPlane, 0.1f, 5.0f);
                ImGui::SliderFloat("Far Plane", &light.ShadowFarPlane, 5.0f, 100.0f);
                
                float range = light.ShadowFarPlane - light.ShadowNearPlane;
                ImGui::Text("Shadow Range: %.1f units", range);
            }
        }
        
        ImGui::TreePop();
    }
}

const char* LightInspector::GetLightTypeIcon(LightType type)
{
    switch (type)
    {
        case LightType::Directional: return "(Dir)";
        case LightType::Point: return "(Pnt)";
        case LightType::Spot: return "(Spt)";
        default: return "?";
    }
}
