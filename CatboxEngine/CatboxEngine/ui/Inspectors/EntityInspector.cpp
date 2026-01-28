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
    // diffuse color edit via mesh handle
    float color[3] = {0.8f,0.8f,0.9f};
    if (entity.MeshHandle != 0)
    {
        Mesh* m = MeshManager::Instance().GetMesh(entity.MeshHandle);
        if (m) { color[0] = m->DiffuseColor.x; color[1] = m->DiffuseColor.y; color[2] = m->DiffuseColor.z; }
    }
    if (ImGui::ColorEdit3("Diffuse Color", color))
    {
        if (entity.MeshHandle != 0)
        {
            Mesh* m = MeshManager::Instance().GetMesh(entity.MeshHandle);
            if (m) { m->DiffuseColor.x = color[0]; m->DiffuseColor.y = color[1]; m->DiffuseColor.z = color[2]; }
        }
    }

    // Texture preview and change
    ImGui::Separator();
    ImGui::Text("Diffuse Texture");
    // show path from mesh handle
    std::string diffPath = "(none)";
    if (entity.MeshHandle != 0)
    {
        Mesh* m = MeshManager::Instance().GetMesh(entity.MeshHandle);
        if (m) diffPath = m->DiffuseTexturePath.empty() ? "(none)" : m->DiffuseTexturePath;
    }
    ImGui::Text("%s", diffPath.c_str());
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
            if (entity.MeshHandle != 0)
            {
                Mesh* m = MeshManager::Instance().GetMesh(entity.MeshHandle);
                if (m) { m->LoadTexture(sel); m->DiffuseTexturePath = sel; }
            }
            }
        }
    if (ImGui::MenuItem("Specular"))
    {
        char buf[1024] = {0};
        if (Platform::OpenFileDialog(buf, (int)sizeof(buf), "Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0All\0*.*\0"))
        {
            std::string sel(buf);
            if (entity.MeshHandle != 0)
            {
                Mesh* m = MeshManager::Instance().GetMesh(entity.MeshHandle);
                if (m) { m->LoadSpecularTexture(sel); m->SpecularTexturePath = sel; }
            }
        }
    }
    if (ImGui::MenuItem("Normal"))
    {
        char buf[1024] = {0};
        if (Platform::OpenFileDialog(buf, (int)sizeof(buf), "Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0All\0*.*\0"))
        {
            std::string sel(buf);
            if (entity.MeshHandle != 0)
            {
                Mesh* m = MeshManager::Instance().GetMesh(entity.MeshHandle);
                if (m) { m->LoadNormalTexture(sel); m->NormalTexturePath = sel; }
            }
        }
    }
        ImGui::EndPopup();
    }
    if (entity.MeshHandle != 0)
    {
        Mesh* m = MeshManager::Instance().GetMesh(entity.MeshHandle);
        if (m)
        {
            if (m->HasDiffuseTexture) ImGui::Image((ImTextureID)(uintptr_t)m->DiffuseTexture, ImVec2(128,128));
            ImGui::Text("Specular: %.2f,%.2f,%.2f", m->SpecularColor.x, m->SpecularColor.y, m->SpecularColor.z);
            ImGui::Text("Shininess: %.2f", m->Shininess);
            ImGui::Text("Alpha: %.2f", m->Alpha);
            if (m->HasSpecularTexture)
            {
                ImGui::Text("Specular Map: %s", m->SpecularTexturePath.c_str());
                ImGui::Image((ImTextureID)(uintptr_t)m->SpecularTexture, ImVec2(64,64));
            }
            if (m->HasNormalTexture)
            {
                ImGui::Text("Normal Map: %s", m->NormalTexturePath.c_str());
                ImGui::Image((ImTextureID)(uintptr_t)m->NormalTexture, ImVec2(64,64));
            }
        }
    }

    ImGui::End();
}
