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

    // Texture preview and change
    ImGui::Separator();
    ImGui::Text("Diffuse Texture");
    ImGui::Text("%s", entity.Mesh.DiffuseTexturePath.empty() ? "(none)" : entity.Mesh.DiffuseTexturePath.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Change Texture"))
    {
        // show popup to pick which map
        ImGui::OpenPopup("ChangeTexturePopup");
    }
    if (ImGui::BeginPopup("ChangeTexturePopup"))
    {
        if (ImGui::MenuItem("Diffuse"))
        {
            char buf[1024] = {0};
            if (Platform::OpenFileDialog(buf, (int)sizeof(buf), "Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0All\0*.*\0"))
            {
                std::string sel(buf);
                if (entity.Mesh.LoadTexture(sel)) entity.Mesh.DiffuseTexturePath = sel;
            }
        }
        if (ImGui::MenuItem("Specular"))
        {
            char buf[1024] = {0};
            if (Platform::OpenFileDialog(buf, (int)sizeof(buf), "Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0All\0*.*\0"))
            {
                std::string sel(buf);
                if (entity.Mesh.LoadSpecularTexture(sel)) entity.Mesh.SpecularTexturePath = sel;
            }
        }
        if (ImGui::MenuItem("Normal"))
        {
            char buf[1024] = {0};
            if (Platform::OpenFileDialog(buf, (int)sizeof(buf), "Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0All\0*.*\0"))
            {
                std::string sel(buf);
                if (entity.Mesh.LoadNormalTexture(sel)) entity.Mesh.NormalTexturePath = sel;
            }
        }
        ImGui::EndPopup();
    }
    if (entity.Mesh.HasDiffuseTexture)
    {
        // show small preview (ImGui::Image requires a texture ID cast)
        ImGui::Image((ImTextureID)(uintptr_t)entity.Mesh.DiffuseTexture, ImVec2(128,128));
    }
    ImGui::Text("Specular: %.2f,%.2f,%.2f", entity.Mesh.SpecularColor.x, entity.Mesh.SpecularColor.y, entity.Mesh.SpecularColor.z);
    ImGui::Text("Shininess: %.2f", entity.Mesh.Shininess);
    ImGui::Text("Alpha: %.2f", entity.Mesh.Alpha);
    if (entity.Mesh.HasSpecularTexture)
    {
        ImGui::Text("Specular Map: %s", entity.Mesh.SpecularTexturePath.c_str());
        ImGui::Image((ImTextureID)(uintptr_t)entity.Mesh.SpecularTexture, ImVec2(64,64));
    }
    if (entity.Mesh.HasNormalTexture)
    {
        ImGui::Text("Normal Map: %s", entity.Mesh.NormalTexturePath.c_str());
        ImGui::Image((ImTextureID)(uintptr_t)entity.Mesh.NormalTexture, ImVec2(64,64));
    }

    ImGui::End();
}
