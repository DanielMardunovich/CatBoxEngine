#pragma once
#include "../../resources/Math/Vec3.h"
#include <string>

class EntityManager;

class SpawnInspector
{
public:
    SpawnInspector() = default;
    ~SpawnInspector() = default;

    // Disable copy, enable move
    SpawnInspector(const SpawnInspector&) = delete;
    SpawnInspector& operator=(const SpawnInspector&) = delete;
    SpawnInspector(SpawnInspector&&) noexcept = default;
    SpawnInspector& operator=(SpawnInspector&&) noexcept = default;

    void Draw(EntityManager& entityManager, Vec3& spawnPosition, Vec3& spawnScale, 
             int selectedIndex, bool& useSharedCube);

private:
    void SpawnNewEntity(EntityManager& entityManager, const Vec3& spawnPosition,
                       const Vec3& spawnScale, bool useSharedCube);
    void ApplyModelToSelected(EntityManager& entityManager, int selectedIndex);
    void DrawTextureAssignmentPopup(EntityManager& entityManager, int selectedIndex);
    void DrawModelErrorPopup();
    
    // State
    char m_modelPath[260] = "";
    std::string m_pendingTexturePath;
    bool m_showModelError = false;
    char m_modelErrorMsg[512] = "";
};
