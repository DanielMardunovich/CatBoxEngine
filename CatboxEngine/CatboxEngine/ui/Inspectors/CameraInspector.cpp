#include "CameraInspector.h"
#include "../../resources/Camera.h"
#include "imgui.h"

void CameraInspector::Draw(Camera& camera)
{
    ImGui::Begin("Camera Inspector");

    // Field of View
    float fov = camera.FOV;
    if (ImGui::SliderFloat("FOV", &fov, 10.0f, 120.0f))
    {
        camera.FOV = fov;
    }

    // Near and Far planes
    float nearPlane = camera.Near;
    if (ImGui::InputFloat("Near Plane", &nearPlane))
    {
        camera.Near = nearPlane;
    }

    float farPlane = camera.Far;
    if (ImGui::InputFloat("Far Plane", &farPlane))
    {
        camera.Far = farPlane;
    }

    // Mouse sensitivity
    float sensitivity = camera.MouseSensitivity;
    if (ImGui::SliderFloat("Mouse Sensitivity", &sensitivity, 0.01f, 1.0f))
    {
        camera.MouseSensitivity = sensitivity;
    }

    // Position
    ImGui::Separator();
    ImGui::Text("Transform");
    ImGui::InputFloat3("Position", &camera.Position.x);

    // Reset button
    ImGui::Separator();
    if (ImGui::Button("Reset Camera"))
    {
        camera.Initialize({0.0f, 0.0f, 3.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 
                         60.0f, camera.Aspect, 0.1f, 100.0f);
    }

    ImGui::End();
}
