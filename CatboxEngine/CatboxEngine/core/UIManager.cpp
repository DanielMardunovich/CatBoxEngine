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
#include "../ui/Inspectors/LightInspector.h"
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
    m_lightInspector = new LightInspector();
}

UIManager::~UIManager()
{
    // Clean up inspectors
    delete m_cameraInspector;
    delete m_entityManagerInspector;
    delete m_statsInspector;
    delete m_lightInspector;
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
    // Entity Manager Inspector
    m_entityManagerInspector->Draw(entityManager, spawnPosition, spawnScale, 
                                   selectedIndex, useSharedCube);

    // Statistics Inspector
    m_statsInspector->Draw(deltaTime, entityManager);

    // Camera Inspector
    m_cameraInspector->Draw(camera);
    
    // Light Inspector
    m_lightInspector->Draw();
    
    // Scene Manager
    DrawSceneManager(entityManager);
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
