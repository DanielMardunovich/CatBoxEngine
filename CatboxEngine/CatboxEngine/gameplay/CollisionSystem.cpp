#include "CollisionSystem.h"
#include "TerrainSystem.h"
#include "../resources/EntityManager.h"
#include "../resources/Entity.h"
#include "../graphics/MeshManager.h"
#include "../graphics/Mesh.h"
#include <algorithm>

CollisionSystem::AABB CollisionSystem::ComputeAABB(const Entity& entity)
{
    glm::vec3 pos(entity.Transform.Position.x,
                  entity.Transform.Position.y,
                  entity.Transform.Position.z);

    // Use the mesh's real bounding box when available.
    // This ensures foot-origin models (origin at y=0) sit flush on surfaces
    // instead of floating half a scale-unit above them.
    if (entity.MeshHandle != 0)
    {
        const Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
        if (mesh && mesh->BoundsMin.x != FLT_MAX && mesh->BoundsMax.x != -FLT_MAX)
        {
            glm::vec3 scaledMin(mesh->BoundsMin.x * entity.Transform.Scale.x,
                                mesh->BoundsMin.y * entity.Transform.Scale.y,
                                mesh->BoundsMin.z * entity.Transform.Scale.z);
            glm::vec3 scaledMax(mesh->BoundsMax.x * entity.Transform.Scale.x,
                                mesh->BoundsMax.y * entity.Transform.Scale.y,
                                mesh->BoundsMax.z * entity.Transform.Scale.z);
            // glm::min/max handles negative scales gracefully
            return { pos + glm::min(scaledMin, scaledMax),
                     pos + glm::max(scaledMin, scaledMax) };
        }
    }

    // Fallback: assume a unit cube centred at the origin
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

bool CollisionSystem::ResolveTerrainCollisions(Entity& player, glm::vec3& velocity,
                                               const EntityManager& entityManager)
{
    bool isGrounded = false;

    // Work out where the player's feet are relative to their Position origin.
    // For a unit-cube player the offset is -Scale.y/2; for a foot-origin model it is 0.
    float feetOffset = -player.Transform.Scale.y * 0.5f;  // fallback
    if (player.MeshHandle != 0)
    {
        const Mesh* mesh = MeshManager::Instance().GetMesh(player.MeshHandle);
        if (mesh && mesh->BoundsMin.y != FLT_MAX)
            feetOffset = mesh->BoundsMin.y * player.Transform.Scale.y;
    }

    for (const auto& entity : entityManager.GetAll())
    {
        if (!entity.IsTerrain || entity.TerrainHeightData.empty())
            continue;

        const float terrainY = TerrainSystem::SampleHeight(
            entity, player.Transform.Position.x, player.Transform.Position.z);

        if (terrainY < -1e10f)  // -FLT_MAX sentinel: player outside terrain XZ bounds
            continue;

        const float playerFeetY = player.Transform.Position.y + feetOffset;
        static constexpr float k_groundProbe = 0.12f;

        if (playerFeetY <= terrainY + k_groundProbe)
        {
            if (playerFeetY < terrainY)
            {
                // Push player up so their feet land exactly on the surface
                player.Transform.Position.y = terrainY - feetOffset;
                if (velocity.y < 0.0f)
                    velocity.y = 0.0f;
            }
            isGrounded = true;
        }
    }

    return isGrounded;
}
