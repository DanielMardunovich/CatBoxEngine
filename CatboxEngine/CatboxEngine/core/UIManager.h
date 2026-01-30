#pragma once
#include "../resources/Math/Vec3.h"

class EntityManager;
class Camera;
class SceneManager;
class EntityInspector;
class CameraInspector;
class SpawnInspector;
class EntityListInspector;
class StatsInspector;

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
             float deltaTime, int& selectedIndex, Camera& camera, bool& useSharedCube);
    
    // Render ImGui draw data
    void Render();

private:
    // Draw scene manager window
    void DrawSceneManager(EntityManager& entityManager);
    
    // Draw lights window
    void DrawLightsWindow();

    // Inspectors (owned by UIManager)
    EntityInspector* m_entityInspector = nullptr;
    CameraInspector* m_cameraInspector = nullptr;
    SpawnInspector* m_spawnInspector = nullptr;
    EntityListInspector* m_entityListInspector = nullptr;
    StatsInspector* m_statsInspector = nullptr;

    // Selected light index (-1 = none selected)
    int selectedLightIndex = -1;
};
