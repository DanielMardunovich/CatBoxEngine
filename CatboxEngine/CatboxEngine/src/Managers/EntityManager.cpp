#include "EntityManager.h"

EntityManager& EntityManager::Get()
{
    static EntityManager instance;
    return instance;
}

void EntityManager::CreateEntity()
{   
    std::lock_guard<std::mutex> lock(mtx);

    Entity* entity = new Entity();

    entities.push_back(entity);
}

std::list<Entity*> EntityManager::GetAllEntities()
{
    std::lock_guard<std::mutex> lock(mtx);
    return entities;
}
