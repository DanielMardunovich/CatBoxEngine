#pragma once
#include <mutex>
#include <list>
#include <string>
#include <glm/glm.hpp>
#include "../Renderable.h"

struct Entity {
    Entity() {}
    ~Entity() {}

    std::string name = "Entity";
    std::string modelPath;
    std::string texturePath;

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f); // Euler degrees

    // Non-owning pointer to the renderable associated with this entity (may be null)
    Renderable* renderable = nullptr;
};

class EntityManager
{
public:
    static EntityManager& Get();

    // Creates a new entity and automatically creates/attaches a Renderable (Cube) at the entity position.
    // The EntityManager keeps ownership of the Entity. Renderables are registered with RenderManager.
    Entity* CreateEntity(const glm::vec3& position = glm::vec3(0.0f));

    // Destroys the entity and also removes and deletes its associated renderable (if any).
    void DestroyEntity(Entity* entity);

    // Returns a copy of the list of entity pointers (thread-safe).
    std::list<Entity*> GetAllEntities() const;

private:
    EntityManager() = default;

    std::list<Entity*> entities;
    mutable std::mutex mtx;
};