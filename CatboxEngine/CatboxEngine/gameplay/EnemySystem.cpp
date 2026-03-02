#include "EnemySystem.h"
#include "../resources/EntityManager.h"
#include "../resources/Entity.h"
#include "PlayerController.h"
#include <cmath>
#include <iostream>

namespace
{
    constexpr float WAYPOINT_REACH_THRESHOLD = 0.2f;
}

void EnemySystem::EnterPlayMode(EntityManager& entityManager)
{
    m_states.clear();
    m_originalPositions.clear();

    auto& entities = entityManager.GetAll();
    for (int i = 0; i < static_cast<int>(entities.size()); ++i)
    {
        auto& enemy = entities[i];
        if (!enemy.IsEnemy || enemy.PatrolWaypoints.empty())
            continue;

        // Save original editor position for restoration on exit
        m_originalPositions[i] = enemy.Transform.Position;

        // Snap to first waypoint so patrol always begins from a known position
        enemy.Transform.Position = enemy.PatrolWaypoints[0];
        m_states[i] = EnemyState{};
    }
}

void EnemySystem::ExitPlayMode(EntityManager& entityManager)
{
    auto& entities = entityManager.GetAll();
    for (auto& kv : m_originalPositions)
    {
        int idx = kv.first;
        if (idx < static_cast<int>(entities.size()))
            entities[idx].Transform.Position = kv.second;
    }

    m_states.clear();
    m_originalPositions.clear();
}

void EnemySystem::Update(EntityManager& entityManager, PlayerController& playerController, float deltaTime)
{
    if (!playerController.HasPlayerEntity())
        return;

    Vec3 playerPos = playerController.GetPlayerPosition();
    auto& entities = entityManager.GetAll();

    for (int i = 0; i < static_cast<int>(entities.size()); ++i)
    {
        auto& enemy = entities[i];
        if (!enemy.IsEnemy || enemy.PatrolWaypoints.empty())
            continue;

        // Lazy-initialise state if not yet present
        if (m_states.find(i) == m_states.end())
            m_states[i] = EnemyState{};

        EnemyState& state = m_states[i];
        const int waypointCount = static_cast<int>(enemy.PatrolWaypoints.size());

        // Clamp index into valid range
        if (state.waypointIndex >= waypointCount)
            state.waypointIndex = 0;
        if (state.waypointIndex < 0)
            state.waypointIndex = 0;

        const Vec3& target = enemy.PatrolWaypoints[state.waypointIndex];

        // Direction vector from enemy to current target waypoint
        float dx = target.x - enemy.Transform.Position.x;
        float dy = target.y - enemy.Transform.Position.y;
        float dz = target.z - enemy.Transform.Position.z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (dist < WAYPOINT_REACH_THRESHOLD)
        {
            // Advance to the next waypoint according to the patrol mode
            if (enemy.EnemyPatrolMode == PatrolMode::Loop)
            {
                state.waypointIndex = (state.waypointIndex + 1) % waypointCount;
            }
            else  // PingPong
            {
                state.waypointIndex += state.direction;
                if (state.waypointIndex >= waypointCount)
                {
                    state.direction = -1;
                    state.waypointIndex = waypointCount - 2;
                    if (state.waypointIndex < 0)
                        state.waypointIndex = 0;
                }
                else if (state.waypointIndex < 0)
                {
                    state.direction = 1;
                    state.waypointIndex = 1;
                    if (state.waypointIndex >= waypointCount)
                        state.waypointIndex = 0;
                }
            }
        }
        else
        {
            // Move toward the current target waypoint
            float step = enemy.EnemySpeed * deltaTime;
            if (step > dist)
                step = dist;

            enemy.Transform.Position.x += (dx / dist) * step;
            enemy.Transform.Position.y += (dy / dist) * step;
            enemy.Transform.Position.z += (dz / dist) * step;
        }

        // Check whether the player is within the enemy's collision radius
        float pdx = playerPos.x - enemy.Transform.Position.x;
        float pdy = playerPos.y - enemy.Transform.Position.y;
        float pdz = playerPos.z - enemy.Transform.Position.z;
        float playerDistSq = pdx * pdx + pdy * pdy + pdz * pdz;
        float radiusSq = enemy.EnemyCollisionRadius * enemy.EnemyCollisionRadius;

        if (playerDistSq <= radiusSq)
        {
            Entity* spawnPoint = entityManager.FindSpawnPoint();
            if (spawnPoint)
            {
                playerController.TeleportTo(spawnPoint->Transform.Position);
                std::cout << "Player hit by enemy \"" << enemy.name
                          << "\" - respawning at spawn point" << std::endl;
            }
        }
    }
}
