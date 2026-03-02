#include "CollisionSystem.h"
#include "../resources/EntityManager.h"
#include "../resources/Entity.h"
#include <algorithm>

CollisionSystem::AABB CollisionSystem::ComputeAABB(const Entity& entity)
{
    glm::vec3 pos(entity.Transform.Position.x,
                  entity.Transform.Position.y,
                  entity.Transform.Position.z);
    glm::vec3 half(entity.Transform.Scale.x * 0.5f,
                   entity.Transform.Scale.y * 0.5f,
                   entity.Transform.Scale.z * 0.5f);
    return { pos - half, pos + half };
}

bool CollisionSystem::TestAABBOverlap(const AABB& a, const AABB& b)
{
    return (a.Min.x <= b.Max.x && a.Max.x >= b.Min.x) &&
           (a.Min.y <= b.Max.y && a.Max.y >= b.Min.y) &&
           (a.Min.z <= b.Max.z && a.Max.z >= b.Min.z);
}

bool CollisionSystem::ResolvePlayerCollisions(Entity& player, glm::vec3& velocity,
                                              const EntityManager& entityManager)
{
    bool isGrounded = false;
    const auto& entities = entityManager.GetAll();

    // Up to 3 resolution passes for stability when touching multiple surfaces
    // simultaneously (e.g. a floor corner or a wall-floor junction).
    for (int pass = 0; pass < 3; ++pass)
    {
        AABB playerBox = ComputeAABB(player);
        bool hadCollision = false;

        for (const auto& entity : entities)
        {
            if (&entity == &player)
                continue;
            if (!entity.CollidesWithPlayer)
                continue;

            AABB entityBox = ComputeAABB(entity);
            if (!TestAABBOverlap(playerBox, entityBox))
                continue;

            hadCollision = true;

            // Penetration depth on each axis
            float overlapX = std::min(playerBox.Max.x, entityBox.Max.x) -
                             std::max(playerBox.Min.x, entityBox.Min.x);
            float overlapY = std::min(playerBox.Max.y, entityBox.Max.y) -
                             std::max(playerBox.Min.y, entityBox.Min.y);
            float overlapZ = std::min(playerBox.Max.z, entityBox.Max.z) -
                             std::max(playerBox.Min.z, entityBox.Min.z);

            glm::vec3 playerCenter = (playerBox.Min + playerBox.Max) * 0.5f;
            glm::vec3 entityCenter = (entityBox.Min + entityBox.Max) * 0.5f;

            // Resolve along the axis of minimum penetration (MTV)
            if (overlapY <= overlapX && overlapY <= overlapZ)
            {
                // Vertical contact
                if (playerCenter.y > entityCenter.y)
                {
                    // Player is above: push up (floor/platform landing)
                    player.Transform.Position.y += overlapY;
                    if (velocity.y < 0.0f)
                        velocity.y = 0.0f;
                    isGrounded = true;
                }
                else
                {
                    // Player is below: push down (ceiling hit)
                    player.Transform.Position.y -= overlapY;
                    if (velocity.y > 0.0f)
                        velocity.y = 0.0f;
                }
            }
            else if (overlapX <= overlapZ)
            {
                // X-axis wall contact
                if (playerCenter.x > entityCenter.x)
                {
                    player.Transform.Position.x += overlapX;
                    if (velocity.x < 0.0f)
                        velocity.x = 0.0f;
                }
                else
                {
                    player.Transform.Position.x -= overlapX;
                    if (velocity.x > 0.0f)
                        velocity.x = 0.0f;
                }
            }
            else
            {
                // Z-axis wall contact
                if (playerCenter.z > entityCenter.z)
                {
                    player.Transform.Position.z += overlapZ;
                    if (velocity.z < 0.0f)
                        velocity.z = 0.0f;
                }
                else
                {
                    player.Transform.Position.z -= overlapZ;
                    if (velocity.z > 0.0f)
                        velocity.z = 0.0f;
                }
            }

            // Recompute after each resolved contact so subsequent entities in
            // the same pass use the corrected position.
            playerBox = ComputeAABB(player);
        }

        if (!hadCollision)
            break;
    }

    // Ground probe: detect the player standing exactly on top of a surface
    // without needing to be penetrating it (handles the steady-state case).
    if (!isGrounded)
    {
        static constexpr float k_groundProbe = 0.12f;
        AABB playerBox = ComputeAABB(player);

        for (const auto& entity : entities)
        {
            if (&entity == &player)
                continue;
            if (!entity.CollidesWithPlayer)
                continue;

            AABB entityBox = ComputeAABB(entity);
            float playerBottom = playerBox.Min.y;
            float entityTop    = entityBox.Max.y;

            // Entity surface must be just at or below the player's feet
            if (entityTop <= playerBottom && entityTop >= playerBottom - k_groundProbe)
            {
                // Confirm horizontal overlap so the player is actually above it
                if (playerBox.Min.x < entityBox.Max.x && playerBox.Max.x > entityBox.Min.x &&
                    playerBox.Min.z < entityBox.Max.z && playerBox.Max.z > entityBox.Min.z)
                {
                    isGrounded = true;
                    break;
                }
            }
        }
    }

    return isGrounded;
}
