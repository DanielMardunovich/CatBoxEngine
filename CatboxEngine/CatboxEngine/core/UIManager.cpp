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
#include "../ui/Inspectors/GraphicsSettingsInspector.h"
#include "../ui/Inspectors/PlayerInspector.h"
#include "../ui/Inspectors/LevelSelectMenu.h"
#include "../gameplay/RecordTimeSystem.h"
#include "../graphics/MeshManager.h"
#include "../graphics/LightManager.h"
#include <glad/glad.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>

UIManager::UIManager()
{
    m_cameraInspector           = TRACKED_NEW(CameraInspector);
    m_entityManagerInspector    = TRACKED_NEW(EntityManagerInspector);
    m_statsInspector            = TRACKED_NEW(StatsInspector);
    m_lightInspector            = TRACKED_NEW(LightInspector);
    m_graphicsSettingsInspector = TRACKED_NEW(GraphicsSettingsInspector);
    m_playerInspector           = TRACKED_NEW(PlayerInspector);
    m_levelSelectMenu           = TRACKED_NEW(LevelSelectMenu);
}

UIManager::~UIManager()
{
    TRACKED_DELETE(m_cameraInspector);
    TRACKED_DELETE(m_entityManagerInspector);
    TRACKED_DELETE(m_statsInspector);
    TRACKED_DELETE(m_lightInspector);
    TRACKED_DELETE(m_graphicsSettingsInspector);
    TRACKED_DELETE(m_playerInspector);
    TRACKED_DELETE(m_levelSelectMenu);
}

void UIManager::NewFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::Draw(EntityManager& entityManager, Vec3& spawnPosition, Vec3& spawnScale, 
                    float deltaTime, int& selectedIndex, Camera& camera, bool& useSharedCube,
                    PlayerController* playerController, bool& isPlayMode,
                    bool goalReached, RecordTimeSystem* recordSystem)
{
    bool playerReady = playerController && playerController->HasPlayerEntity();
    DrawPlayModeToolbar(isPlayMode, playerReady);

    if (isPlayMode && goalReached)
        DrawGoalOverlay();

    if (isPlayMode && recordSystem && !goalReached)
        DrawTimerHUD(recordSystem->GetCurrentTime());

    if (!isPlayMode)
    {
        m_entityManagerInspector->Draw(entityManager, spawnPosition, spawnScale,
                                       selectedIndex, useSharedCube);
        m_cameraInspector->Draw(camera);
        m_lightInspector->Draw();
        m_graphicsSettingsInspector->Draw();
        DrawSceneManager(entityManager, recordSystem);
    }

    m_statsInspector->Draw(deltaTime, entityManager);

    if (playerController && !isPlayMode)
        m_playerInspector->Draw(*playerController, entityManager, camera);
}

void UIManager::NotifyGoalResult(float completionTime, bool isNewBest)
{
    m_completionTime = completionTime;
    m_isNewBest      = isNewBest;
}

void UIManager::DrawGoalOverlay()
{
    ImGuiIO& io = ImGui::GetIO();

    // Semi-transparent full-screen dim layer
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.55f);
    ImGui::Begin("##GoalDim", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoInputs    | ImGuiWindowFlags_NoSavedSettings);
    ImGui::End();

    // Centred goal card — taller when we have a time to show
    const float cardW = 420.0f;
    const float cardH = (m_completionTime >= 0.0f) ? 270.0f : 220.0f;
    ImGui::SetNextWindowPos(
        ImVec2((io.DisplaySize.x - cardW) * 0.5f, (io.DisplaySize.y - cardH) * 0.5f),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(cardW, cardH), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.97f);
    ImGui::Begin("##GoalCard", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar  | ImGuiWindowFlags_NoSavedSettings);

    // Star row
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.1f, 1.0f));
    const char* stars = "  * * * * * * *";
    ImGui::SetCursorPosX((cardW - ImGui::CalcTextSize(stars).x) * 0.5f);
    ImGui::Text("%s", stars);
    ImGui::PopStyleColor();

    // Heading
    ImGui::Spacing();
    ImGui::SetWindowFontScale(2.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.2f, 1.0f));
    const char* heading = "GOAL REACHED!";
    ImGui::SetCursorPosX((cardW - ImGui::CalcTextSize(heading).x) * 0.5f);
    ImGui::Text("%s", heading);
    ImGui::PopStyleColor();
    ImGui::SetWindowFontScale(1.0f);

    // Completion time / new best
    if (m_completionTime >= 0.0f)
    {
        ImGui::Spacing();
        std::string timeStr = RecordTimeSystem::FormatTime(m_completionTime);
        if (m_isNewBest)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.4f, 1.0f));
            std::string label = "NEW BEST!  " + timeStr;
            ImGui::SetCursorPosX((cardW - ImGui::CalcTextSize(label.c_str()).x) * 0.5f);
            ImGui::Text("%s", label.c_str());
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
            std::string label = "Time:  " + timeStr;
            ImGui::SetCursorPosX((cardW - ImGui::CalcTextSize(label.c_str()).x) * 0.5f);
            ImGui::Text("%s", label.c_str());
            ImGui::PopStyleColor();
        }
    }

    // Sub-message
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.85f, 1.0f));
    const char* sub = "You reached the goal!";
    ImGui::SetCursorPosX((cardW - ImGui::CalcTextSize(sub).x) * 0.5f);
    ImGui::Text("%s", sub);
    ImGui::PopStyleColor();

    // Exit hint
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    const char* hint = "Press ESC or Stop to return to the editor";
    ImGui::SetCursorPosX((cardW - ImGui::CalcTextSize(hint).x) * 0.5f);
    ImGui::Text("%s", hint);
    ImGui::PopStyleColor();

    ImGui::End();
}

void UIManager::DrawTimerHUD(float elapsed)
{
    ImGuiIO& io = ImGui::GetIO();
    std::string timeStr = RecordTimeSystem::FormatTime(elapsed);

    ImGui::SetNextWindowBgAlpha(0.55f);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10.0f, 40.0f),
                            ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::Begin("##TimerHUD", nullptr,
        ImGuiWindowFlags_NoDecoration     | ImGuiWindowFlags_NoMove    |
        ImGuiWindowFlags_NoInputs         | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::SetWindowFontScale(1.4f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.3f, 1.0f));
    ImGui::Text("%s", timeStr.c_str());
    ImGui::PopStyleColor();
    ImGui::SetWindowFontScale(1.0f);

    ImGui::End();
}

void UIManager::Render()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::DrawPlayModeToolbar(bool& isPlayMode, bool playerReady)
{
    ImGuiIO& io = ImGui::GetIO();
    const float toolbarWidth = 220.0f;
    ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - toolbarWidth) * 0.5f, 5.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(toolbarWidth, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.92f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
                           | ImGuiWindowFlags_NoScrollbar  | ImGuiWindowFlags_AlwaysAutoResize
                           | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("##PlayToolbar", nullptr, flags);

    if (isPlayMode)
    {
        // Stop button
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.75f, 0.15f, 0.15f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.25f, 0.25f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.60f, 0.10f, 0.10f, 1.00f));
        if (ImGui::Button("  Stop  ", ImVec2(-1.0f, 0.0f)))
            isPlayMode = false;
        ImGui::PopStyleColor(3);

        ImGui::SetItemTooltip("Exit Play Mode  [ESC]");
    }
    else
    {
        // Play button
        if (!playerReady) ImGui::BeginDisabled();

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.65f, 0.15f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.80f, 0.25f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.10f, 0.50f, 0.10f, 1.00f));
        if (ImGui::Button("  Play  ", ImVec2(-1.0f, 0.0f)))
            isPlayMode = true;
        ImGui::PopStyleColor(3);

        if (!playerReady)
        {
            ImGui::EndDisabled();
            ImGui::SetItemTooltip("No player entity assigned.\nUse the Player Controller window to set one up.");
        }
        else
        {
            ImGui::SetItemTooltip("Enter Play Mode");
        }
    }

    ImGui::End();
}

void UIManager::DrawSceneManager(EntityManager& entityManager, RecordTimeSystem* records)
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

    // ── Level Select ─────────────────────────────────────────────────────────
    if (records)
    {
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Level Select"))
        {
            ImGui::Spacing();
            m_levelSelectMenu->DrawContents(entityManager, *records);
            ImGui::Spacing();
        }
    }

    ImGui::End();
}
