#pragma once
#include "Entity.h"
#include <vector>

class EntityManager
{
public:
    void Add(const Entity& e) { m_entities.push_back(e); }
    const std::vector<Entity>& GetAll() const { return m_entities; }
    std::vector<Entity>& GetAll() { return m_entities; }
    void RemoveAt(size_t idx) { if (idx < m_entities.size()) m_entities.erase(m_entities.begin() + idx); }
    void Clear() { m_entities.clear(); }
    size_t Size() const { return m_entities.size(); }

private:
    std::vector<Entity> m_entities;
};
