#include "EntityInspector.h"
#include "imgui.h"

void EntityInspector::Draw(Entity& entity)
{
    if (&entity == nullptr) return;

    ImGui::Begin("Inspector");
    ImGui::Text("Entity: %s", entity.name.c_str());

    ImGui::Separator();
    ImGui::Text("Transform");
    ImGui::InputFloat3("Position", &entity.Transform.Position.x);
    ImGui::InputFloat3("Rotation", &entity.Transform.Rotation.x);
    ImGui::InputFloat3("Scale", &entity.Transform.Scale.x);

    ImGui::End();
}
