#pragma once
#include<mutex>
#include <list>

struct Entity {
    Entity() {}
	~Entity() {}

};

class EntityManager
{
public:
    static EntityManager& Get();

    void CreateEntity();

    std::list<Entity*> GetAllEntities();

private:
    EntityManager() = default;

	std::list<Entity*> entities;

    mutable std::mutex mtx;
};


