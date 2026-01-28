#pragma once
#include "../resources/Math/Vec3.h"

class EntityManager;
class Camera;

class UIManager
{
public:
    UIManager() = default;

    // Start new ImGui frame
    void NewFrame();

    // Build UI (spawning, listing entities, stats)
    // selectedIndex is an in/out parameter: UI may change selection
    void Draw(EntityManager& entityManager, Vec3& spawnPosition, Vec3& spawnScale, float deltaTime, int& selectedIndex, Camera& camera, bool& useSharedCube);

    // Render ImGui draw data
    void Render();
};
