#include "PlayerInspector.h"
#include "imgui.h"

void PlayerInspector::Draw(PlayerController& playerController, EntityManager& entityManager, Camera& camera)
{
    ImGui::Begin("Player Controller");

    // Enable/Disable controller
    bool enabled = playerController.IsEnabled();
    if (ImGui::Checkbox("Controller Enabled", &enabled))
    {
        playerController.SetEnabled(enabled);
    }

    ImGui::Separator();

    // Player entity selection
    DrawPlayerSelection(playerController, entityManager, camera);
    
    ImGui::Separator();
    
    // Player stats
    DrawPlayerStats(playerController);
    
    ImGui::Separator();
    
    // Movement settings
    if (ImGui::CollapsingHeader("Movement Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawMovementSettings(playerController);
    }
    
    ImGui::Separator();
    
    // Camera settings
    if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawCameraSettings(playerController);
    }
    
    ImGui::Separator();
    
    // Controls help
    if (ImGui::CollapsingHeader("Controls"))
    {
        DrawControls();
    }
    
    ImGui::End();
}

void PlayerInspector::DrawPlayerSelection(PlayerController& playerController, EntityManager& entityManager, Camera& camera)
{
    ImGui::Text("Player Entity Setup");

    auto& entities = entityManager.GetAll();

    // Show currently assigned player
    if (playerController.HasPlayerEntity())
    {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Player assigned");
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.1f, 1.0f), "No player assigned");
    }

    if (entities.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No entities available!");
        ImGui::Text("Create an entity first to use as player.");
        return;
    }

    // Entity selection dropdown
    std::string previewText = (m_selectedPlayerEntityIndex >= 0 && m_selectedPlayerEntityIndex < static_cast<int>(entities.size()))
        ? entities[m_selectedPlayerEntityIndex].name
        : "Select Entity...";

    if (ImGui::BeginCombo("Player Entity", previewText.c_str()))
    {
        for (size_t i = 0; i < entities.size(); ++i)
        {
            bool isSelected = (m_selectedPlayerEntityIndex == static_cast<int>(i));
            if (ImGui::Selectable(entities[i].name.c_str(), isSelected))
            {
                m_selectedPlayerEntityIndex = static_cast<int>(i);
            }

            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Create player button
    ImGui::SameLine();
    if (ImGui::Button("Create Player Cube"))
    {
        Entity playerEntity;
        playerEntity.name = "Player";
        playerEntity.Transform.Position = Vec3(0.0f, 2.0f, 0.0f);
        playerEntity.Transform.Scale = Vec3(0.5f, 1.0f, 0.5f);

        entityManager.AddEntity(playerEntity, true);
        // Entity was just appended, so it is at size-1
        m_selectedPlayerEntityIndex = static_cast<int>(entityManager.GetAll().size()) - 1;
    }

    // Initialize button — only shown when an entity is selected
    if (m_selectedPlayerEntityIndex >= 0 && m_selectedPlayerEntityIndex < static_cast<int>(entities.size()))
    {
        if (ImGui::Button("Assign as Player"))
        {
            Entity* playerEntity = &entityManager.GetAll()[m_selectedPlayerEntityIndex];
            playerController.Initialize(playerEntity, &camera);
        }
        ImGui::SetItemTooltip("Set '%s' as the player entity and snap the 3rd-person camera behind it.",
                              entities[m_selectedPlayerEntityIndex].name.c_str());
    }
}

void PlayerInspector::DrawPlayerStats(const PlayerController& playerController)
{
    ImGui::Text("Player Stats");
    
    // State
    const char* stateStr = "Unknown";
    switch (playerController.GetState())
    {
        case PlayerState::Idle: stateStr = "Idle"; break;
        case PlayerState::Walking: stateStr = "Walking"; break;
        case PlayerState::Running: stateStr = "Running"; break;
        case PlayerState::Jumping: stateStr = "Jumping"; break;
        case PlayerState::Falling: stateStr = "Falling"; break;
        case PlayerState::Landing: stateStr = "Landing"; break;
    }
    ImGui::Text("State: %s", stateStr);
    
    // Grounded
    ImGui::Text("Grounded: %s", playerController.IsGrounded() ? "Yes" : "No");
    
    // Velocity
    glm::vec3 vel = playerController.GetVelocity();
    ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", vel.x, vel.y, vel.z);
    
    float speed = glm::length(glm::vec2(vel.x, vel.z));
    ImGui::Text("Horizontal Speed: %.2f", speed);
    
    // Camera angles
    ImGui::Text("Camera Yaw: %.1f°", playerController.GetCameraYaw());
    ImGui::Text("Camera Pitch: %.1f°", playerController.GetCameraPitch());
}

void PlayerInspector::DrawMovementSettings(PlayerController& playerController)
{
    auto& config = playerController.MovementConfig;
    
    ImGui::SliderFloat("Walk Speed", &config.WalkSpeed, 1.0f, 15.0f);
    ImGui::SliderFloat("Run Speed", &config.RunSpeed, 5.0f, 25.0f);
    ImGui::SliderFloat("Acceleration", &config.Acceleration, 5.0f, 50.0f);
    ImGui::SliderFloat("Deceleration", &config.Deceleration, 5.0f, 50.0f);
    ImGui::SliderFloat("Turn Speed", &config.TurnSpeed, 180.0f, 1440.0f, "%.0f°/s");
    ImGui::SliderFloat("Jump Force", &config.JumpForce, 3.0f, 15.0f);
    ImGui::SliderFloat("Gravity", &config.Gravity, 5.0f, 40.0f);
    
    if (ImGui::Button("Reset to Defaults"))
    {
        config.WalkSpeed = 5.0f;
        config.RunSpeed = 10.0f;
        config.Acceleration = 20.0f;
        config.Deceleration = 15.0f;
        config.TurnSpeed = 720.0f;
        config.JumpForce = 8.0f;
        config.Gravity = 20.0f;
    }
}

void PlayerInspector::DrawCameraSettings(PlayerController& playerController)
{
    auto& config = playerController.CameraConfig;
    
    ImGui::SliderFloat("Distance", &config.Distance, 2.0f, 20.0f);
    ImGui::SliderFloat("Height", &config.Height, 0.0f, 8.0f);
    ImGui::SliderFloat("Sensitivity", &config.Sensitivity, 0.05f, 0.5f);
    ImGui::SliderFloat("Min Pitch", &config.MinPitch, -89.0f, 0.0f, "%.0f°");
    ImGui::SliderFloat("Max Pitch", &config.MaxPitch, 0.0f, 89.0f, "%.0f°");
    ImGui::SliderFloat("Smooth Speed", &config.SmoothSpeed, 1.0f, 20.0f);
    
    if (ImGui::Button("Reset to Defaults"))
    {
        config.Distance = 8.0f;
        config.Height = 3.0f;
        config.Sensitivity = 0.15f;
        config.MinPitch = -30.0f;
        config.MaxPitch = 60.0f;
        config.SmoothSpeed = 10.0f;
    }
}

void PlayerInspector::DrawControls()
{
    ImGui::Text("Movement:");
    ImGui::BulletText("W/A/S/D - Move");
    ImGui::BulletText("Shift - Run");
    ImGui::BulletText("Space - Jump");
    
    ImGui::Spacing();
    ImGui::Text("Camera:");
    ImGui::BulletText("Right Mouse Button - Hold to rotate camera");
    ImGui::BulletText("Mouse Move - Look around");
    
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), 
        "Tip: The camera is relative to player movement,\njust like Super Mario 64!");
}
