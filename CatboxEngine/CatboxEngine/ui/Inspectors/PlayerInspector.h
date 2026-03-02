#pragma once
#include "../../gameplay/PlayerController.h"
#include "../../resources/EntityManager.h"
#include "../../resources/Camera.h"

class PlayerInspector
{
public:
    PlayerInspector() = default;

    void Draw(PlayerController& playerController, EntityManager& entityManager, Camera& camera);

private:
    void DrawPlayerSelection(PlayerController& playerController, EntityManager& entityManager, Camera& camera);
    void DrawPlayerStats(const PlayerController& playerController);
    void DrawMovementSettings(PlayerController& playerController);
    void DrawCameraSettings(PlayerController& playerController);
    void DrawControls();

    int m_selectedPlayerEntityIndex = -1;
};
