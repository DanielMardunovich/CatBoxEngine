#include "CameraInspector.h"
#include "../../resources/Camera.h"
#include "imgui.h"

void CameraInspector::Draw(Camera& camera)
{
    ImGui::Begin("Camera Inspector");

    float fov = camera.FOV;
    if (ImGui::SliderFloat("FOV", &fov, 10.0f, 120.0f)) camera.FOV = fov;

    float nearP = camera.Near;
    if (ImGui::InputFloat("Near", &nearP)) camera.Near = nearP;
    float farP = camera.Far;
    if (ImGui::InputFloat("Far", &farP)) camera.Far = farP;

    float sens = camera.MouseSensitivity;
    if (ImGui::SliderFloat("Mouse Sensitivity", &sens, 0.01f, 1.0f)) camera.MouseSensitivity = sens;

    ImGui::InputFloat3("Position", &camera.Position.x);

    if (ImGui::Button("Reset Camera"))
    {
        camera.Initialize({0,0,3}, {0,0,0}, {0,1,0}, 60.0f, camera.Aspect, 0.1f, 100.0f);
    }

    ImGui::End();
}
