#pragma once
#include "Entity.h"
#include "../graphics/MeshManager.h"
#include <vector>

class EntityManager
{
public:
    void Add(const Entity& e) { m_entities.push_back(e); }
    // add entity and ensure it has a mesh (cube if none). returns index
    int AddEntity(const Entity& e, bool useSharedCube);
    const std::vector<Entity>& GetAll() const { return m_entities; }
    std::vector<Entity>& GetAll() { return m_entities; }
    void RemoveAt(size_t idx) { if (idx < m_entities.size()) { auto h = m_entities[idx].MeshHandle; if (h != 0) MeshManager::Instance().Release(h); m_entities.erase(m_entities.begin() + idx); } }
    void Clear() { m_entities.clear(); }
    size_t Size() const { return m_entities.size(); }

private:
    std::vector<Entity> m_entities;
};

