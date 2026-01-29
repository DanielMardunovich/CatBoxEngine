#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Entity.h"
#include "Camera.h"
#include "Math/Vec3.h"
#include "../graphics/Light.h"  // Include Light structure

// Forward declare to avoid circular dependency
class EntityManager;

// Scene represents a complete game state with entities, camera, and settings
class Scene
{
public:
    Scene(const std::string& name = "Untitled Scene");
    ~Scene();

    // Scene lifecycle
    void OnLoad(EntityManager& entityManager);      // Called when scene becomes active
    void OnUnload(EntityManager& entityManager);    // Called when scene is deactivated
    void Update(float deltaTime);
    
    // Sync with EntityManager
    void CaptureFromEntityManager(EntityManager& entityManager);  // Save current state
    
    // Scene properties
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }
    
    bool IsLoaded() const { return m_isLoaded; }
    
    // Entity management
    void AddEntity(const Entity& entity);
    void RemoveEntity(size_t index);
    Entity* GetEntity(size_t index);
    const std::vector<Entity>& GetEntities() const { return m_entities; }
    size_t GetEntityCount() const { return m_entities.size(); }
    void ClearEntities();
    
    // Camera
    Camera& GetCamera() { return m_camera; }
    const Camera& GetCamera() const { return m_camera; }
    void SetCamera(const Camera& camera) { m_camera = camera; }
    
    // Lights (modern system)
    void AddLight(const Light& light) { m_lights.push_back(light); }
    void RemoveLight(size_t index);
    const std::vector<Light>& GetLights() const { return m_lights; }
    std::vector<Light>& GetLights() { return m_lights; }
    void ClearLights() { m_lights.clear(); }
    size_t GetLightCount() const { return m_lights.size(); }
    
    // Environment
    Vec3 AmbientColor{0.1f, 0.1f, 0.1f};
    Vec3 BackgroundColor{0.4f, 0.3f, 0.2f};
    
    // Serialization
    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);
    
    // Scene metadata
    struct Metadata
    {
        std::string author;
        std::string description;
        std::string version = "1.0";
        long long createdTime = 0;
        long long modifiedTime = 0;
    };
    
    Metadata& GetMetadata() { return m_metadata; }
    const Metadata& GetMetadata() const { return m_metadata; }

private:
    std::string m_name;
    bool m_isLoaded = false;
    
    std::vector<Entity> m_entities;
    std::vector<Light> m_lights;  // Store scene lights
    Camera m_camera;
    Metadata m_metadata;
};
