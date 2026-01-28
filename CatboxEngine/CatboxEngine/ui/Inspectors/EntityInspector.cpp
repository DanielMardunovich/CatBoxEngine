#include "EntityInspector.h"
#include "imgui.h"

void EntityInspector::Draw(Entity& entity)
{
    ImGui::Begin("Inspector");

    // Name (editable)
    char nameBuf[128];
    strncpy_s(nameBuf, entity.name.c_str(), sizeof(nameBuf));
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
    {
        entity.name = std::string(nameBuf);
    }

    ImGui::Separator();
    ImGui::Text("Transform");
    ImGui::InputFloat3("Position", &entity.Transform.Position.x);
    ImGui::InputFloat3("Rotation", &entity.Transform.Rotation.x);
    ImGui::InputFloat3("Scale", &entity.Transform.Scale.x);

    // Material / Mesh properties
    ImGui::Separator();
    ImGui::Text("Material");
    // show and edit diffuse color if mesh present
    float color[3] = { entity.Mesh.DiffuseColor.x, entity.Mesh.DiffuseColor.y, entity.Mesh.DiffuseColor.z };
    if (ImGui::ColorEdit3("Diffuse Color", color))
    {
        entity.Mesh.DiffuseColor.x = color[0];
        entity.Mesh.DiffuseColor.y = color[1];
        entity.Mesh.DiffuseColor.z = color[2];
    }

    ImGui::End();
}
