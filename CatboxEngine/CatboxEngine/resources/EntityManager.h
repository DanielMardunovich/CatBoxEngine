#pragma once
#include "Entity.h"
#include "../graphics/MeshManager.h"
#include "../core/MessageQueue.h"
#include <vector>
#include <memory>

class EntityManager
{
public:
    void Add(const Entity& e) { m_entities.push_back(e); }
    // add entity and ensure it has a mesh (cube if none). returns index
    int AddEntity(const Entity& e, bool useSharedCube);
    const std::vector<Entity>& GetAll() const { return m_entities; }
    std::vector<Entity>& GetAll() { return m_entities; }
    
    void RemoveAt(size_t idx)
    {
        if (idx < m_entities.size())
        {
            auto h = m_entities[idx].MeshHandle;
            std::string name = m_entities[idx].name;
            
            if (h != 0) MeshManager::Instance().Release(h);
            m_entities.erase(m_entities.begin() + idx);
            
            // Post entity destroyed message
            auto msg = std::make_shared<EntityDestroyedMessage>((int)idx, name);
            MessageQueue::Instance().Post(msg);
        }
    }
    
    void Clear()
    {
        for (auto& e : m_entities)
            if (e.MeshHandle != 0) MeshManager::Instance().Release(e.MeshHandle);
        m_entities.clear();
    }
    size_t Size() const { return m_entities.size(); }

    // Returns the first entity tagged as a spawn point, or nullptr
    Entity* FindSpawnPoint()
    {
        for (auto& e : m_entities)
            if (e.IsSpawnPoint) return &e;
        return nullptr;
    }

    // Returns the entity marked as the player entity, or nullptr
    Entity* FindPlayerEntity()
    {
        for (auto& e : m_entities)
            if (e.IsPlayer) return &e;
        return nullptr;
    }

    // Returns a unique pair ID for a new teleporter pair
    int GetNextTeleporterPairID() { return m_nextTeleporterPairID++; }

    // After loading a scene, ensure new pairs don't reuse any loaded pair ID
    void SyncTeleporterPairID(int loadedMaxPairID)
    {
        if (loadedMaxPairID >= m_nextTeleporterPairID)
            m_nextTeleporterPairID = loadedMaxPairID + 1;
    }

private:
    std::vector<Entity> m_entities;
    int m_nextTeleporterPairID = 0;
};

