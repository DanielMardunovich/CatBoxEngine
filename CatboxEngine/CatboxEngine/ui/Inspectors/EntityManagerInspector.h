#pragma once
#include "../../resources/Math/Vec3.h"
#include <string>

class EntityManager;
class Camera;
class Entity;

class EntityManagerInspector
{
public:
    EntityManagerInspector() = default;
    ~EntityManagerInspector() = default;

    // Disable copy, enable move
    EntityManagerInspector(const EntityManagerInspector&) = delete;
    EntityManagerInspector& operator=(const EntityManagerInspector&) = delete;
    EntityManagerInspector(EntityManagerInspector&&) noexcept = default;
    EntityManagerInspector& operator=(EntityManagerInspector&&) noexcept = default;

    void Draw(EntityManager& entityManager, Vec3& spawnPosition, Vec3& spawnScale,
             int& selectedIndex, bool& useSharedCube);

private:
    // Spawn controls
    void DrawSpawnControls(EntityManager& entityManager, Vec3& spawnPosition, 
                          Vec3& spawnScale, int selectedIndex, bool useSharedCube);
    void SpawnNewEntity(EntityManager& entityManager, const Vec3& spawnPosition,
                       const Vec3& spawnScale, bool useSharedCube);
    void ApplyModelToSelected(EntityManager& entityManager, int selectedIndex);
    
    // Entity list
    void DrawEntityList(EntityManager& entityManager, int& selectedIndex);
    
    // Full entity inspector (when selected)
    void DrawFullEntityInspector(EntityManager& entityManager, int selectedIndex);
    void DrawEntityInfo(Entity& entity);
    void DrawEntityTransform(Entity& entity);
    void DrawEntityMesh(Entity& entity);
    void DrawEntityMaterial(Entity& entity);
    void DrawEntityTextures(Entity& entity);
    
    // Texture management helpers
    enum class TextureType { Diffuse, Specular, Normal };
    void DrawTextureOverride(Entity& entity, TextureType type);
    unsigned int LoadTextureWithSettings(const char* path, int& width, int& height, int channels);
    
    // Popups
    void DrawTextureAssignmentPopup(EntityManager& entityManager, int selectedIndex);
    void DrawModelErrorPopup();
    
    // State
    char m_modelPath[260] = "";
    std::string m_pendingTexturePath;
    bool m_showModelError = false;
    char m_modelErrorMsg[512] = "";
};
