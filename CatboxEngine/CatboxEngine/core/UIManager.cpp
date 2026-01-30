#include "UIManager.h"
#include "Platform.h"
#include "MemoryTracker.h"
#include "../resources/SceneManager.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "../resources/EntityManager.h"
#include "../resources/Entity.h"
#include "../resources/Camera.h"
#include "../ui/Inspectors/CameraInspector.h"
#include "../ui/Inspectors/EntityManagerInspector.h"
#include "../ui/Inspectors/StatsInspector.h"
#include "../graphics/MeshManager.h"
#include "../graphics/LightManager.h"
#include <glad/glad.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>

UIManager::UIManager()
{
    // Create inspectors
    m_cameraInspector = new CameraInspector();
    m_entityManagerInspector = new EntityManagerInspector();
    m_statsInspector = new StatsInspector();
}

UIManager::~UIManager()
{
    // Clean up inspectors
    delete m_cameraInspector;
    delete m_entityManagerInspector;
    delete m_statsInspector;
}

void UIManager::NewFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::Draw(EntityManager& entityManager, Vec3& spawnPosition, Vec3& spawnScale, 
                    float deltaTime, int& selectedIndex, Camera& camera, bool& useSharedCube)
{
    // Entity Manager Inspector (unified: spawn + list + embedded entity inspector)
    m_entityManagerInspector->Draw(entityManager, spawnPosition, spawnScale, 
                                   selectedIndex, useSharedCube);

    // Statistics Inspector (separate window)
    m_statsInspector->Draw(deltaTime, entityManager);

    // Camera Inspector
    m_cameraInspector->Draw(camera);
    
    // Scene Manager
    DrawSceneManager(entityManager);
    
    // Lights Window
    DrawLightsWindow();
}

void UIManager::Render()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::DrawSceneManager(EntityManager& entityManager)
{
    auto& sceneMgr = SceneManager::Instance();
    
    ImGui::Begin("Scene Manager");
    
    ImGui::Text("Scenes: %zu", sceneMgr.GetSceneCount());
    ImGui::Separator();
    
    // Current scene info
    auto activeScene = sceneMgr.GetActiveScene();
    if (activeScene)
    {
        ImGui::Text("Active Scene: %s", activeScene->GetName().c_str());
        ImGui::Text("Entities: %zu (in scene) / %zu (in manager)", 
            activeScene->GetEntityCount(), entityManager.Size());
        ImGui::Text("Loaded: %s", activeScene->IsLoaded() ? "Yes" : "No");
    }
    else
    {
        ImGui::TextDisabled("No active scene");
    }
    
    ImGui::Separator();
    
    // Create new scene
    static char newSceneName[128] = "New Scene";
    ImGui::InputText("##NewSceneName", newSceneName, sizeof(newSceneName));
    ImGui::SameLine();
    if (ImGui::Button("Create Scene"))
    {
        SceneID id = sceneMgr.CreateScene(newSceneName);
        // Optionally activate it
        if (sceneMgr.GetSceneCount() == 1)
        {
            sceneMgr.SetActiveScene(id, entityManager);
        }
    }
    
    ImGui::Separator();
    
    // Scene list
    auto sceneIDs = sceneMgr.GetAllSceneIDs();
    if (!sceneIDs.empty())
    {
        ImGui::Text("All Scenes:");
        ImGui::BeginChild("SceneList", ImVec2(0, 150), true);
        
        for (SceneID id : sceneIDs)
        {
            auto scene = sceneMgr.GetScene(id);
            if (!scene) continue;
            
            bool isActive = (id == sceneMgr.GetActiveSceneID());
            
            ImGui::PushID((int)id);
            
            if (isActive)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
            }
            
            // Show entity count next to name
            std::string label = scene->GetName() + " (" + std::to_string(scene->GetEntityCount()) + ")";
            if (ImGui::Selectable(label.c_str(), isActive))
            {
                sceneMgr.SetActiveScene(id, entityManager);
            }
            
            if (isActive)
            {
                ImGui::PopStyleColor();
            }
            
            // Context menu
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Save..."))
                {
                    char path[260] = {0};
                    if (Platform::SaveFileDialog(path, sizeof(path), "Scene Files\0*.scene\0All\0*.*\0"))
                    {
                        sceneMgr.SaveScene(id, path, entityManager);
                    }
                }
                
                if (ImGui::MenuItem("Unload", nullptr, false, !isActive))
                {
                    sceneMgr.UnloadScene(id, entityManager);
                }
                
                ImGui::EndPopup();
            }
            
            ImGui::PopID();
        }
        
        ImGui::EndChild();
    }
    
    ImGui::Separator();
    
    // File operations
    if (ImGui::Button("Load Scene..."))
    {
        char path[260] = {0};
        if (Platform::OpenFileDialog(path, sizeof(path), "Scene Files\0*.scene\0All\0*.*\0"))
        {
            SceneID id = sceneMgr.LoadScene(path);
            if (id != 0)
            {
                sceneMgr.SetActiveScene(id, entityManager);
            }
        }
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Save Active Scene..."))
    {
        if (activeScene)
        {
            char path[260] = {0};
            if (Platform::SaveFileDialog(path, sizeof(path), "Scene Files\0*.scene\0All\0*.*\0"))
            {
                sceneMgr.SaveScene(sceneMgr.GetActiveSceneID(), path, entityManager);
            }
        }
    }
    
    ImGui::End();
}

void UIManager::DrawLightsWindow()
{
    auto& lightMgr = LightManager::Instance();
    
    ImGui::Begin("Lights");
    
    ImGui::Text("Active Lights: %zu", lightMgr.GetLightCount());
    ImGui::Separator();
    
    // Spawn position for new lights
    static Vec3 lightSpawnPos = {0, 5, 0};
    ImGui::DragFloat3("Spawn Position", &lightSpawnPos.x, 0.1f);
    
    // Spawn buttons
    if (ImGui::Button("Spawn Point Light"))
    {
        Light newLight;
        newLight.Name = "Point Light " + std::to_string(lightMgr.GetLightCount());
        newLight.Type = LightType::Point;
        newLight.Position = lightSpawnPos;
        newLight.Color = {1, 1, 1};
        newLight.Intensity = 1.0f;
        newLight.CastsShadows = false;  // Default to no shadows for performance
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
        newLight.Position = lightSpawnPos;
        newLight.Direction = {0, -1, 0};
        newLight.Color = {1, 1, 1};
        newLight.Intensity = 1.0f;
        newLight.CastsShadows = false;
        lightMgr.AddLight(newLight);
    }
    
    ImGui::Separator();
    
    // List all lights
    ImGui::Text("Light List:");
    ImGui::BeginChild("LightList", ImVec2(0, 200), true);
    
    auto& lights = lightMgr.GetAllLights();
    ImGui::Columns(2);
    ImGui::SetColumnWidth(1, 90.0f);
    
    for (size_t i = 0; i < lights.size(); ++i)
    {
        ImGui::PushID((int)i);
        
        auto& light = lights[i];
        bool isSelected = (selectedLightIndex == (int)i);
        
        // Left column: selectable name with type indicator
        std::string displayName = light.Name;
        if (!light.Enabled)
            displayName += " (Disabled)";
        
        const char* typeIcon = "?";
        if (light.Type == LightType::Directional) typeIcon = "?";
        else if (light.Type == LightType::Point) typeIcon = "??";
        else if (light.Type == LightType::Spot) typeIcon = "??";
        
        displayName = std::string(typeIcon) + " " + displayName;
        
        if (ImGui::Selectable(displayName.c_str(), isSelected))
        {
            selectedLightIndex = (int)i;
        }
        
        ImGui::NextColumn();
        
        // Right column: delete button
        ImGui::AlignTextToFramePadding();
        if (ImGui::SmallButton("Delete"))
        {
            lightMgr.RemoveLight(i);
            if (selectedLightIndex == (int)i) 
                selectedLightIndex = -1;
            else if (selectedLightIndex > (int)i) 
                selectedLightIndex -= 1;
            ImGui::PopID();
            break;
        }
        
        ImGui::NextColumn();
        ImGui::PopID();
    }
    
    ImGui::Columns(1);
    ImGui::EndChild();
    
    ImGui::Separator();
    
    // Show light inspector for selected light
    if (selectedLightIndex >= 0 && selectedLightIndex < (int)lights.size())
    {
        auto& light = lights[selectedLightIndex];
        
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
        
        // Light type
        const char* types[] = { "Directional", "Point", "Spot" };
        int currentType = (int)light.Type;
        if (ImGui::Combo("Type", &currentType, types, 3))
        {
            light.Type = (LightType)currentType;
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
                
                // Create or destroy shadow map
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
                    // Destroy shadow map
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
                else // Point or Spot light
                {
                    ImGui::SliderFloat("Shadow FOV", &light.ShadowFOV, 60.0f, 179.0f, "%.0f°");
                    ImGui::SliderFloat("Near Plane", &light.ShadowNearPlane, 0.1f, 5.0f);
                    ImGui::SliderFloat("Far Plane", &light.ShadowFarPlane, 5.0f, 100.0f);
                    
                    // Show effective coverage
                    float range = light.ShadowFarPlane - light.ShadowNearPlane;
                    ImGui::Text("Shadow Range: %.1f units", range);
                }
            }
            
            ImGui::TreePop();
        }
    }
    
    ImGui::End();
}
