#include "TeleporterSystem.h"
#include "../resources/EntityManager.h"
#include "PlayerController.h"
#include <iostream>

void TeleporterSystem::Update(EntityManager& entityManager, PlayerController& playerController, float deltaTime)
{
    if (!playerController.HasPlayerEntity())
        return;

    // Tick down all cooldowns
    for (auto& kv : m_cooldowns)
        kv.second -= deltaTime;

    Vec3 playerPos = playerController.GetPlayerPosition();
    auto& entities = entityManager.GetAll();

    for (size_t i = 0; i < entities.size(); ++i)
    {
        const auto& src = entities[i];
        if (!src.IsTeleporter || src.TeleporterPairID < 0)
            continue;

        // Skip if this pair is on cooldown
        auto it = m_cooldowns.find(src.TeleporterPairID);
        if (it != m_cooldowns.end() && it->second > 0.0f)
            continue;

        // Check if player is within activation radius
        float dx = playerPos.x - src.Transform.Position.x;
        float dy = playerPos.y - src.Transform.Position.y;
        float dz = playerPos.z - src.Transform.Position.z;
        float distSq = dx * dx + dy * dy + dz * dz;
        if (distSq > src.TeleporterRadius * src.TeleporterRadius)
            continue;

        // Find the linked partner (same pair ID, different entity)
        for (size_t j = 0; j < entities.size(); ++j)
        {
            if (i == j) continue;
            const auto& dest = entities[j];
            if (!dest.IsTeleporter || dest.TeleporterPairID != src.TeleporterPairID)
                continue;

            // Teleport player to destination, offset upward to avoid floor clipping
            Vec3 destPos = dest.Transform.Position;
            destPos.y += 1.0f;
            playerController.TeleportTo(destPos);

            // Put the whole pair on cooldown so the player doesn't immediately bounce back
            m_cooldowns[src.TeleporterPairID] = CooldownDuration;

            std::cout << "Teleported via pair " << src.TeleporterPairID
                      << ": " << src.name << " -> " << dest.name << std::endl;
            return; // One teleport per frame is enough
        }
    }
}

void TeleporterSystem::Reset()
{
    m_cooldowns.clear();
}
