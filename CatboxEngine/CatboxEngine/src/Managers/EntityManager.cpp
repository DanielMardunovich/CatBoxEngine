#include "EntityManager.h"
#include "../Managers/RenderManager.h"
#include "../Cube.h"
#include <iostream>

EntityManager& EntityManager::Get()
{
    static EntityManager instance;
    return instance;
}

Entity* EntityManager::CreateEntity(const glm::vec3& position)
{
    Entity* e = new Entity();
    e->position = position;

    {
        std::lock_guard<std::mutex> lock(mtx);
        entities.push_back(e);
    }

    // Automatically create a Cube renderable for this entity and register it with RenderManager.
    // Store the non-owning pointer on the entity so UI can refer to it later.
    Renderable* r = new Cube(position);
    RenderManager::Get().AddRenderable(r);
    e->renderable = r;

    return e;
}

void EntityManager::DestroyEntity(Entity* entity)
{
    if (!entity) return;

    // First remove renderable (if present) from RenderManager so it gets deleted.
    if (entity->renderable)
    {
        RenderManager::Get().RemoveRenderable(entity->renderable);
        entity->renderable = nullptr;
    }

    // Remove entity from list and delete it
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = std::find(entities.begin(), entities.end(), entity);
        if (it != entities.end())
            entities.erase(it);
    }

    delete entity;
}

std::list<Entity*> EntityManager::GetAllEntities() const
{
    std::lock_guard<std::mutex> lock(mtx);
    return entities; // copy of list of pointers
}