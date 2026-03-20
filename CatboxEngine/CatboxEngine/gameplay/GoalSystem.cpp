#include "GoalSystem.h"
#include "../resources/EntityManager.h"
#include "PlayerController.h"
#include "CollisionSystem.h"
#include <iostream>

void GoalSystem::Update(EntityManager& entityManager, PlayerController& playerController)
{
    if (m_goalReached)
        return;

    Entity* playerEntity = playerController.GetPlayerEntity();
    if (!playerEntity)
        return;

    for (const auto& entity : entityManager.GetAll())
    {
        if (!entity.IsGoal)
            continue;

        if (CollisionSystem::IsColliding(*playerEntity, entity))
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
