#pragma once
#include <unordered_map>
#include "../resources/Math/Vec3.h"

class EntityManager;
class PlayerController;

// Moves enemy entities along their configured waypoints and sends the player
// back to the spawn point when they get too close.
class EnemySystem
{
public:
    // Call when entering play mode: saves original positions and snaps enemies to waypoint 0.
    void EnterPlayMode(EntityManager& entityManager);

    // Call when exiting play mode: restores original editor positions.
    void ExitPlayMode(EntityManager& entityManager);

    // Call every frame while in play mode.
    void Update(EntityManager& entityManager, PlayerController& playerController, float deltaTime);

private:
    struct EnemyState
    {
        int waypointIndex = 0;
        int direction = 1;  // +1 or -1, used for PingPong mode
    };

    // Maps entity list index -> runtime patrol state
    std::unordered_map<int, EnemyState> m_states;

    // Saved editor positions so ExitPlayMode can restore them
    std::unordered_map<int, Vec3> m_originalPositions;
};
