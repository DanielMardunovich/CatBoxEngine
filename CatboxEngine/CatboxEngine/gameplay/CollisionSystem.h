#pragma once
#include <glm/glm.hpp>

class EntityManager;
class Entity;

// AABB-based collision system for the player character.
// Resolves penetrations against all scene entities that have CollidesWithPlayer
// enabled and reports whether the player is grounded after resolution.
class CollisionSystem
{
public:
    // Pushes the player entity out of any overlapping collidable entities and
    // zeroes the relevant velocity components at each contact surface.
    // Returns true when the player is resting on solid ground after resolution.
    static bool ResolvePlayerCollisions(Entity& player, glm::vec3& velocity,
                                        const EntityManager& entityManager);

    // Resolves the player standing on (or falling into) terrain entities.
    // Returns true when the player is grounded on terrain.
    static bool ResolveTerrainCollisions(Entity& player, glm::vec3& velocity,
                                         const EntityManager& entityManager);

private:
    struct AABB
    {
        glm::vec3 Min;
        glm::vec3 Max;
    };

    // Builds an AABB from an entity's world position and scale.
    // Assumes the mesh origin is at the centre of the bounding box (unit-cube).
    static AABB ComputeAABB(const Entity& entity);

    // Returns true when two AABBs overlap on all three axes.
    static bool TestAABBOverlap(const AABB& a, const AABB& b);
};
