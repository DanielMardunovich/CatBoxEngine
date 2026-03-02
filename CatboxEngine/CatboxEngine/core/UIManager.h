#pragma once
#include "../resources/Math/Vec3.h"

class EntityManager;
class Camera;
class SceneManager;
class PlayerController;
class RecordTimeSystem;
class CameraInspector;
class EntityManagerInspector;
class StatsInspector;
class LightInspector;
class GraphicsSettingsInspector;
class PlayerInspector;
class LevelSelectMenu;

class UIManager
{
public:
    UIManager();
    ~UIManager();

    // Disable copy, enable move
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;
    UIManager(UIManager&&) noexcept = default;
    UIManager& operator=(UIManager&&) noexcept = default;

    // Start new ImGui frame
    void NewFrame();

    // Build UI (manages all inspectors)
    void Draw(EntityManager& entityManager, Vec3& spawnPosition, Vec3& spawnScale, 
             float deltaTime, int& selectedIndex, Camera& camera, bool& useSharedCube,
             PlayerController* playerController, bool& isPlayMode,
             bool goalReached = false, RecordTimeSystem* recordSystem = nullptr);

    // Called by Engine the frame a goal is first reached
    void NotifyGoalResult(float completionTime, bool isNewBest);

    // Render ImGui draw data
    void Render();

private:
    // Draw scene manager window (includes level select section)
    void DrawSceneManager(EntityManager& entityManager, RecordTimeSystem* records);
    // Draw centered play/stop toolbar
    void DrawPlayModeToolbar(bool& isPlayMode, bool playerReady);
    // Draw goal reached overlay (shows completion time when available)
    void DrawGoalOverlay();
    // Draw the live play-mode timer HUD
    void DrawTimerHUD(float elapsed);

    // Inspectors (owned by UIManager)
    CameraInspector*             m_cameraInspector            = nullptr;
    EntityManagerInspector*      m_entityManagerInspector     = nullptr;
    StatsInspector*              m_statsInspector             = nullptr;
    LightInspector*              m_lightInspector             = nullptr;
    GraphicsSettingsInspector*   m_graphicsSettingsInspector  = nullptr;
    PlayerInspector*             m_playerInspector            = nullptr;
    LevelSelectMenu*             m_levelSelectMenu            = nullptr;

    // Goal result state set by NotifyGoalResult
    float m_completionTime = -1.0f;
    bool  m_isNewBest      = false;
};
