#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "Scene.h"

// Forward declare
class EntityManager;

using SceneID = size_t;

class SceneManager
{
public:
    static SceneManager& Instance()
    {
        static SceneManager instance;
        return instance;
    }

    // Scene management
    SceneID CreateScene(const std::string& name);
    SceneID LoadScene(const std::string& path);
    bool SaveScene(SceneID id, const std::string& path, EntityManager& entityManager);
    bool UnloadScene(SceneID id, EntityManager& entityManager);
    
    // Scene switching
    bool SetActiveScene(SceneID id, EntityManager& entityManager);
    SceneID GetActiveSceneID() const { return m_activeSceneID; }
    Scene* GetActiveScene();
    const Scene* GetActiveScene() const;
    
    // Scene access
    Scene* GetScene(SceneID id);
    const Scene* GetScene(SceneID id) const;
    
    // Query
    size_t GetSceneCount() const { return m_scenes.size(); }
    std::vector<SceneID> GetAllSceneIDs() const;
    std::vector<std::string> GetAllSceneNames() const;
    
    // Update active scene
    void Update(float deltaTime);

private:
    SceneManager() = default;
    ~SceneManager() = default;
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    std::unordered_map<SceneID, std::shared_ptr<Scene>> m_scenes;
    SceneID m_activeSceneID = 0;
    SceneID m_nextSceneID = 1;
};
