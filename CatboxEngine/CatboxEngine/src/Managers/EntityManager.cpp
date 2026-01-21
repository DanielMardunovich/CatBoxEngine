#include "EntityManager.h"
#include "../Entity.h"

EntityManager& EntityManager::Get()
{
    static EntityManager instance;
    return instance;
}



std::list<Entity*> EntityManager::GetAllEntities()
{
    std::lock_guard<std::mutex> lock(mtx);
    return entities;
}
