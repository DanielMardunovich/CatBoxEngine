#pragma once
#include<mutex>
#include <list>
#include "RenderManager.h"

class Entity;

class EntityManager
{
public:
    static EntityManager& Get();

    template<typename T, typename... Args>
    T* CreateEntity(Args&&... args)
    {
        static_assert(std::is_base_of_v<Entity, T>,
            "T must derive from Entity");

        std::lock_guard<std::mutex> lock(mtx);

        T* entity = new T(std::forward<Args>(args)...);

        entities.push_back(entity);

        RenderManager::Get().AddRenderable(entity);

        return entity;
    }

    std::list<Entity*> GetAllEntities();

private:
    EntityManager() = default;

	std::list<Entity*> entities;

    mutable std::mutex mtx;
};


