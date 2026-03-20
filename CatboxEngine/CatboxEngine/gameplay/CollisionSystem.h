#pragma once
#include <glm/glm.hpp>

class EntityManager;
class Entity;

// Collision system for the player character.
// Uses OBB (Oriented Bounding Box) tests so rotated entities produce correct
// contact normals, allowing the player to walk up ramps and slide along walls.
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

    // Returns true when two entities overlap according to the collision system.
    static bool IsColliding(const Entity& a, const Entity& b);

private:
    struct AABB
    {
        glm::vec3 Min;
        glm::vec3 Max;
    };

    struct OBB
    {
        glm::vec3 Center;
        glm::vec3 HalfExtents;
        glm::mat3 Axes;  // columns are the local X, Y, Z axes in world space
    };

    // Builds an axis-aligned AABB (used for broad-phase and player box).
    static AABB ComputeAABB(const Entity& entity);

    // Builds an OBB that respects the entity's rotation.
    static OBB ComputeOBB(const Entity& entity);

    // Returns true when two AABBs overlap on all three axes.
    static bool TestAABBOverlap(const AABB& a, const AABB& b);

    // SAT test between player AABB and entity OBB.
    // On overlap, writes the minimum translation vector (direction * depth)
    // into 'mtv' and the contact normal into 'normal'.
    static bool TestAABBvsOBB(const AABB& aabb, const OBB& obb,
                              glm::vec3& mtv, glm::vec3& normal);
};
