#pragma once
#include <unordered_map>

class EntityManager;
class PlayerController;

class TeleporterSystem
{
public:
    float CooldownDuration = 2.0f;  // seconds before the same pair can fire again

    // Call every frame while in play mode
    void Update(EntityManager& entityManager, PlayerController& playerController, float deltaTime);

    // Call when leaving play mode to clear all active cooldowns
    void Reset();

private:
    // pairID -> remaining cooldown time (seconds)
    std::unordered_map<int, float> m_cooldowns;
};
