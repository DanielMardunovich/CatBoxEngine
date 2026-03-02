#pragma once

class EntityManager;
class PlayerController;

class GoalSystem
{
public:
    void Update(EntityManager& entityManager, PlayerController& playerController);
    void Reset();
    [[nodiscard]] bool IsGoalReached() const { return m_goalReached; }

private:
    bool m_goalReached = false;
};
