#include "CollisionSystem.h"
#include "TerrainSystem.h"
#include "../resources/EntityManager.h"
#include "../resources/Entity.h"
#include "../graphics/MeshManager.h"
#include "../graphics/Mesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// AABB (broad-phase + player box)
// ---------------------------------------------------------------------------
CollisionSystem::AABB CollisionSystem::ComputeAABB(const Entity& entity)
{
    glm::vec3 pos(entity.Transform.Position.x,
                  entity.Transform.Position.y,
                  entity.Transform.Position.z);

    if (entity.MeshHandle != 0)
    {
        const Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
        if (mesh && mesh->BoundsMin.x != FLT_MAX && mesh->BoundsMax.x != -FLT_MAX)
        {
            glm::mat4 rot(1.0f);
            rot = glm::rotate(rot, glm::radians(entity.Transform.Rotation.x), glm::vec3(1,0,0));
            rot = glm::rotate(rot, glm::radians(entity.Transform.Rotation.y), glm::vec3(0,1,0));
            rot = glm::rotate(rot, glm::radians(entity.Transform.Rotation.z), glm::vec3(0,0,1));
            glm::mat3 R(rot);

            glm::vec3 lo(mesh->BoundsMin.x * entity.Transform.Scale.x,
                         mesh->BoundsMin.y * entity.Transform.Scale.y,
                         mesh->BoundsMin.z * entity.Transform.Scale.z);
            glm::vec3 hi(mesh->BoundsMax.x * entity.Transform.Scale.x,
                         mesh->BoundsMax.y * entity.Transform.Scale.y,
                         mesh->BoundsMax.z * entity.Transform.Scale.z);
            glm::vec3 sMin = glm::min(lo, hi);
            glm::vec3 sMax = glm::max(lo, hi);

            glm::vec3 center  = (sMin + sMax) * 0.5f;
            glm::vec3 extents = (sMax - sMin) * 0.5f;
            glm::vec3 rotCenter = R * center;

            glm::vec3 newExtents;
            for (int i = 0; i < 3; ++i)
                newExtents[i] = std::abs(R[0][i]) * extents.x
                              + std::abs(R[1][i]) * extents.y
                              + std::abs(R[2][i]) * extents.z;

            return { pos + rotCenter - newExtents,
                     pos + rotCenter + newExtents };
        }
    }

    glm::vec3 half(entity.Transform.Scale.x * 0.5f,
                   entity.Transform.Scale.y * 0.5f,
                   entity.Transform.Scale.z * 0.5f);
    return { pos - half, pos + half };
}

// ---------------------------------------------------------------------------
// OBB (oriented bounding box for scene entities)
// ---------------------------------------------------------------------------
CollisionSystem::OBB CollisionSystem::ComputeOBB(const Entity& entity)
{
    OBB obb;
    glm::vec3 pos(entity.Transform.Position.x,
                  entity.Transform.Position.y,
                  entity.Transform.Position.z);

    glm::mat4 rot(1.0f);
    rot = glm::rotate(rot, glm::radians(entity.Transform.Rotation.x), glm::vec3(1,0,0));
    rot = glm::rotate(rot, glm::radians(entity.Transform.Rotation.y), glm::vec3(0,1,0));
    rot = glm::rotate(rot, glm::radians(entity.Transform.Rotation.z), glm::vec3(0,0,1));
    obb.Axes = glm::mat3(rot);

    if (entity.MeshHandle != 0)
    {
        const Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
        if (mesh && mesh->BoundsMin.x != FLT_MAX && mesh->BoundsMax.x != -FLT_MAX)
        {
            glm::vec3 lo(mesh->BoundsMin.x * entity.Transform.Scale.x,
                         mesh->BoundsMin.y * entity.Transform.Scale.y,
                         mesh->BoundsMin.z * entity.Transform.Scale.z);
            glm::vec3 hi(mesh->BoundsMax.x * entity.Transform.Scale.x,
                         mesh->BoundsMax.y * entity.Transform.Scale.y,
                         mesh->BoundsMax.z * entity.Transform.Scale.z);
            glm::vec3 sMin = glm::min(lo, hi);
            glm::vec3 sMax = glm::max(lo, hi);

            glm::vec3 localCenter = (sMin + sMax) * 0.5f;
            obb.HalfExtents = (sMax - sMin) * 0.5f;
            obb.Center = pos + obb.Axes * localCenter;
            return obb;
        }
    }

    obb.Center = pos;
    obb.HalfExtents = glm::vec3(entity.Transform.Scale.x * 0.5f,
                                 entity.Transform.Scale.y * 0.5f,
                                 entity.Transform.Scale.z * 0.5f);
    return obb;
}

bool CollisionSystem::TestAABBOverlap(const AABB& a, const AABB& b)
{
    return (a.Min.x <= b.Max.x && a.Max.x >= b.Min.x) &&
           (a.Min.y <= b.Max.y && a.Max.y >= b.Min.y) &&
           (a.Min.z <= b.Max.z && a.Max.z >= b.Min.z);
}

// ---------------------------------------------------------------------------
// SAT test: AABB (player) vs OBB (scene entity)
// Tests 6 face normals (3 AABB + 3 OBB).  Edge-edge cross products are
// intentionally omitted — they are mathematically needed for exact separation
// but in practice they produce near-degenerate axes that cause jitter on
// irregular meshes.  The 6 face normals give stable, gameplay-friendly results.
// ---------------------------------------------------------------------------
bool CollisionSystem::TestAABBvsOBB(const AABB& aabb, const OBB& obb,
                                    glm::vec3& mtv, glm::vec3& normal)
{
    glm::vec3 aCenter  = (aabb.Min + aabb.Max) * 0.5f;
    glm::vec3 aExtents = (aabb.Max - aabb.Min) * 0.5f;

    glm::vec3 d = obb.Center - aCenter;

    // The 3 AABB axes are just the world axes
    glm::vec3 aAxes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
    // The 3 OBB axes
    glm::vec3 bAxes[3] = { obb.Axes[0], obb.Axes[1], obb.Axes[2] };

    float minOverlap = FLT_MAX;
    glm::vec3 minAxis(0.0f);

    auto testAxis = [&](glm::vec3 axis) -> bool {
        float len = glm::length(axis);
        if (len < 1e-6f) return true;
        axis /= len;

        float aProj = aExtents.x * std::abs(glm::dot(aAxes[0], axis))
                    + aExtents.y * std::abs(glm::dot(aAxes[1], axis))
                    + aExtents.z * std::abs(glm::dot(aAxes[2], axis));

        float bProj = obb.HalfExtents.x * std::abs(glm::dot(bAxes[0], axis))
                    + obb.HalfExtents.y * std::abs(glm::dot(bAxes[1], axis))
                    + obb.HalfExtents.z * std::abs(glm::dot(bAxes[2], axis));

        float dist = std::abs(glm::dot(d, axis));
        float overlap = aProj + bProj - dist;

        if (overlap < 0.0f) return false;

        if (overlap < minOverlap)
        {
            minOverlap = overlap;
            minAxis = (glm::dot(d, axis) < 0.0f) ? axis : -axis;
        }
        return true;
    };

    // 3 AABB face normals
    for (int i = 0; i < 3; ++i)
        if (!testAxis(aAxes[i])) return false;

    // 3 OBB face normals
    for (int i = 0; i < 3; ++i)
        if (!testAxis(bAxes[i])) return false;

    // Ignore collisions with negligible penetration to prevent micro-jitter
    if (minOverlap < 0.001f) return false;

    mtv = minAxis * minOverlap;
    normal = minAxis;
    return true;
}

bool CollisionSystem::IsColliding(const Entity& a, const Entity& b)
{
    AABB aabbA = ComputeAABB(a);
    AABB aabbB = ComputeAABB(b);
    if (!TestAABBOverlap(aabbA, aabbB))
        return false;

    glm::vec3 mtv(0.0f);
    glm::vec3 normal(0.0f);

    OBB obbB = ComputeOBB(b);
    if (TestAABBvsOBB(aabbA, obbB, mtv, normal))
        return true;

    OBB obbA = ComputeOBB(a);
    return TestAABBvsOBB(aabbB, obbA, mtv, normal);
}

// ---------------------------------------------------------------------------
// ResolvePlayerCollisions — OBB-aware with vertical bias
// ---------------------------------------------------------------------------
bool CollisionSystem::ResolvePlayerCollisions(Entity& player, glm::vec3& velocity,
                                              const EntityManager& entityManager)
{
    bool isGrounded = false;
    const auto& entities = entityManager.GetAll();

    for (int pass = 0; pass < 4; ++pass)
    {
        AABB playerBox = ComputeAABB(player);
        bool hadCollision = false;

        for (const auto& entity : entities)
        {
            if (&entity == &player) continue;
            if (!entity.CollidesWithPlayer) continue;
            if (entity.IsTerrain) continue;

            // Broad-phase: AABB vs AABB
            AABB entityAABB = ComputeAABB(entity);
            if (!TestAABBOverlap(playerBox, entityAABB)) continue;

            // Narrow-phase: AABB vs OBB
            OBB entityOBB = ComputeOBB(entity);
            glm::vec3 mtvVec, contactNormal;
            if (!TestAABBvsOBB(playerBox, entityOBB, mtvVec, contactNormal))
                continue;

            hadCollision = true;

            // Vertical bias: only when the SAT chose a nearly-horizontal push
            // direction (|normal.y| < 0.3).  This happens on irregular unrotated
            // meshes (e.g. an island) where the bounding box is a poor fit and
            // the X/Z overlap is smaller than Y, causing the SAT to shove the
            // player sideways instead of up.
            //
            // For rotated entities (tilted planks, ramps) the SAT correctly
            // picks the OBB face normal which already has a Y component — so
            // we trust it and skip this override.
            glm::vec3 playerCenter = (playerBox.Min + playerBox.Max) * 0.5f;
            if (playerCenter.y > entityOBB.Center.y && std::abs(contactNormal.y) < 0.3f)
            {
                float yOverlap = entityAABB.Max.y - playerBox.Min.y;
                float playerHeight = playerBox.Max.y - playerBox.Min.y;

                if (yOverlap > 0.0f && yOverlap < playerHeight)
                {
                    mtvVec = glm::vec3(0.0f, yOverlap, 0.0f);
                    contactNormal = glm::vec3(0.0f, 1.0f, 0.0f);
                }
            }

            // Push player out along the contact normal, plus a small
            // skin width to keep the player clearly outside the surface.
            // Without this, floating-point precision causes the next frame's
            // SAT to land on a different axis, flipping the push direction.
            static constexpr float k_skinWidth = 0.002f;
            player.Transform.Position.x += mtvVec.x + contactNormal.x * k_skinWidth;
            player.Transform.Position.y += mtvVec.y + contactNormal.y * k_skinWidth;
            player.Transform.Position.z += mtvVec.z + contactNormal.z * k_skinWidth;

            // Cancel velocity into the surface
            float velIntoSurface = glm::dot(velocity, -contactNormal);
            if (velIntoSurface > 0.0f)
                velocity += contactNormal * velIntoSurface;

            // Ground detection: if the contact normal points mostly upward,
            // the player is standing on this surface.  Also treat any
            // upward push as grounding when the player is above the entity
            // — the SAT can pick a horizontal axis on irregular meshes even
            // though the player is clearly on top.
            if (contactNormal.y > 0.5f
                || (mtvVec.y > 0.001f && playerCenter.y > entityOBB.Center.y))
                isGrounded = true;

            playerBox = ComputeAABB(player);
        }

        if (!hadCollision) break;
    }

    // Ground probe (steady-state detection for flat, sloped, or irregular surfaces)
    if (!isGrounded)
    {
        static constexpr float k_groundProbe = 0.15f;
        AABB playerBox = ComputeAABB(player);
        AABB probeBox  = playerBox;
        probeBox.Min.y -= k_groundProbe;

        for (const auto& entity : entities)
        {
            if (&entity == &player) continue;
            if (!entity.CollidesWithPlayer) continue;
            if (entity.IsTerrain) continue;

            // Broad-phase with the downward-expanded probe box
            AABB entityAABB = ComputeAABB(entity);
            if (!TestAABBOverlap(probeBox, entityAABB)) continue;

            // Only consider entities whose center is below the player
            OBB entityOBB = ComputeOBB(entity);
            glm::vec3 playerCenter = (playerBox.Min + playerBox.Max) * 0.5f;
            if (playerCenter.y <= entityOBB.Center.y) continue;

            // Narrow-phase: test the probe box against the entity OBB
            glm::vec3 probeMtv, probeNormal;
            if (TestAABBvsOBB(probeBox, entityOBB, probeMtv, probeNormal))
            {
                isGrounded = true;
                break;
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
