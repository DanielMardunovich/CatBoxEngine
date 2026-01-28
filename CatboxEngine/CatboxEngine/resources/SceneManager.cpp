#include "SceneManager.h"
#include "EntityManager.h"
#include <iostream>

SceneID SceneManager::CreateScene(const std::string& name)
{
    SceneID id = m_nextSceneID++;
    auto scene = std::make_shared<Scene>(name);
    m_scenes[id] = scene;
    
    std::cout << "Scene created: " << name << " (ID: " << id << ")" << std::endl;
    
    return id;
}

SceneID SceneManager::LoadScene(const std::string& path)
{
    SceneID id = m_nextSceneID++;
    auto scene = std::make_shared<Scene>();
    
    if (!scene->LoadFromFile(path))
    {
        std::cerr << "Failed to load scene from: " << path << std::endl;
        return 0;
    }
    
    m_scenes[id] = scene;
    std::cout << "Scene loaded from file (ID: " << id << ")" << std::endl;
    
    return id;
}

bool SceneManager::SaveScene(SceneID id, const std::string& path, EntityManager& entityManager)
{
    auto it = m_scenes.find(id);
    if (it == m_scenes.end())
    {
        std::cerr << "Scene not found: " << id << std::endl;
        return false;
    }
    
    // Capture current state from EntityManager before saving
    if (id == m_activeSceneID)
    {
        it->second->CaptureFromEntityManager(entityManager);
        std::cout << "Captured current entities to scene before saving" << std::endl;
    }
    
    return it->second->SaveToFile(path);
}

bool SceneManager::UnloadScene(SceneID id, EntityManager& entityManager)
{
    auto it = m_scenes.find(id);
    if (it == m_scenes.end())
    {
        std::cerr << "Scene not found: " << id << std::endl;
        return false;
    }
    
    // Can't unload active scene without switching first
    if (id == m_activeSceneID)
    {
        std::cerr << "Cannot unload active scene. Switch scenes first." << std::endl;
        return false;
    }
    
    // Scene is inactive, just remove it
    m_scenes.erase(it);
    
    std::cout << "Scene unloaded (ID: " << id << ")" << std::endl;
    return true;
}

bool SceneManager::SetActiveScene(SceneID id, EntityManager& entityManager)
{
    auto it = m_scenes.find(id);
    if (it == m_scenes.end())
    {
        std::cerr << "Scene not found: " << id << std::endl;
        return false;
    }
    
    // Unload current scene
    if (m_activeSceneID != 0)
    {
        auto currentScene = GetActiveScene();
        if (currentScene)
        {
            currentScene->OnUnload(entityManager);
        }
    }
    
    // Load new scene
    m_activeSceneID = id;
    it->second->OnLoad(entityManager);
    
    std::cout << "Active scene changed to: " << it->second->GetName() << " (ID: " << id << ")" << std::endl;
    return true;
}

Scene* SceneManager::GetActiveScene()
{
    if (m_activeSceneID == 0) return nullptr;
    return GetScene(m_activeSceneID);
}

const Scene* SceneManager::GetActiveScene() const
{
    if (m_activeSceneID == 0) return nullptr;
    return GetScene(m_activeSceneID);
}

Scene* SceneManager::GetScene(SceneID id)
{
    auto it = m_scenes.find(id);
    if (it != m_scenes.end())
    {
        return it->second.get();
    }
    return nullptr;
}

const Scene* SceneManager::GetScene(SceneID id) const
{
    auto it = m_scenes.find(id);
    if (it != m_scenes.end())
    {
        return it->second.get();
    }
    return nullptr;
}

std::vector<SceneID> SceneManager::GetAllSceneIDs() const
{
    std::vector<SceneID> ids;
    ids.reserve(m_scenes.size());
    for (const auto& pair : m_scenes)
    {
        ids.push_back(pair.first);
    }
    return ids;
}

std::vector<std::string> SceneManager::GetAllSceneNames() const
{
    std::vector<std::string> names;
    names.reserve(m_scenes.size());
    for (const auto& pair : m_scenes)
    {
        names.push_back(pair.second->GetName());
    }
    return names;
}

void SceneManager::Update(float deltaTime)
{
    auto activeScene = GetActiveScene();
    if (activeScene && activeScene->IsLoaded())
    {
        activeScene->Update(deltaTime);
    }
}
