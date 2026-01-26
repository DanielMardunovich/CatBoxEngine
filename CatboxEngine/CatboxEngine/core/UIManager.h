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
    void Draw(EntityManager& entityManager, Vec3& spawnPosition, Vec3& spawnScale, float deltaTime);

    // Render ImGui draw data
    void Render();
};
