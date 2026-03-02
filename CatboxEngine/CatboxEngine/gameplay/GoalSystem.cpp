#include "GoalSystem.h"
#include "../resources/EntityManager.h"
#include "PlayerController.h"
#include <iostream>

void GoalSystem::Update(EntityManager& entityManager, PlayerController& playerController)
{
    if (m_goalReached || !playerController.HasPlayerEntity())
        return;

    Vec3 playerPos = playerController.GetPlayerPosition();

    for (const auto& entity : entityManager.GetAll())
    {
        if (!entity.IsGoal)
            continue;

        float dx = playerPos.x - entity.Transform.Position.x;
        float dy = playerPos.y - entity.Transform.Position.y;
        float dz = playerPos.z - entity.Transform.Position.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq <= entity.GoalRadius * entity.GoalRadius)
        {
            m_goalReached = true;
            std::cout << "Goal reached: " << entity.name << std::endl;
            return;
        }
    }
}

void GoalSystem::Reset()
{
    m_goalReached = false;
}
