#include "CameraInspector.h"
#include "../../resources/Camera.h"
#include "imgui.h"

namespace
{
    // Camera defaults
    constexpr float DEFAULT_FOV = 60.0f;
    constexpr float DEFAULT_NEAR = 0.1f;
    constexpr float DEFAULT_FAR = 100.0f;
    
    // UI limits
    constexpr float MIN_FOV = 10.0f;
    constexpr float MAX_FOV = 120.0f;
    constexpr float MIN_SENSITIVITY = 0.01f;
    constexpr float MAX_SENSITIVITY = 1.0f;
}

void CameraInspector::Draw(Camera& camera)
{
    ImGui::Begin("Camera Inspector");

    // Field of View
    if (ImGui::SliderFloat("FOV", &camera.FOV, MIN_FOV, MAX_FOV))
    {
        
    }

    // Near and Far planes
    ImGui::InputFloat("Near Plane", &camera.Near);
    ImGui::InputFloat("Far Plane", &camera.Far);

    // Mouse sensitivity
    ImGui::SliderFloat("Mouse Sensitivity", &camera.MouseSensitivity, MIN_SENSITIVITY, MAX_SENSITIVITY);

	//Camera speed
    ImGui::InputFloat("Camera Speed", &camera.Speed);

    // Transform
    ImGui::Separator();
    ImGui::Text("Transform");
    ImGui::InputFloat3("Position", &camera.Position.x);

    // Reset button
    ImGui::Separator();
    if (ImGui::Button("Reset Camera"))
    {
        camera.Initialize({0.0f, 0.0f, 3.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 
                         DEFAULT_FOV, camera.Aspect, DEFAULT_NEAR, DEFAULT_FAR, 2.5f);
    }

    ImGui::End();
}
