#include "EntityManagerInspector.h"
#include "../../core/Platform.h"
#include "../../resources/EntityManager.h"
#include "../../resources/Entity.h"
#include "../../graphics/MeshManager.h"
#include "../../graphics/Mesh.h"
#include "../../Dependencies/stb_image.h"
#include "imgui.h"
#include <glad/glad.h>
#include <algorithm>
#include <cctype>
#include <iostream>

void EntityManagerInspector::Draw(EntityManager& entityManager, Vec3& spawnPosition, Vec3& spawnScale,
                                 int& selectedIndex, bool& useSharedCube)
{
    ImGui::Begin("Entity Manager");

    // Header with entity count
    ImGui::Text("Entities: %zu", entityManager.Size());
    ImGui::Separator();

    // Spawn controls at the top
    DrawSpawnControls(entityManager, spawnPosition, spawnScale, selectedIndex, useSharedCube);

    ImGui::Separator();

    // Entity list
    ImGui::Text("Entity List:");
    DrawEntityList(entityManager, selectedIndex);

    ImGui::Separator();

    // Full entity inspector embedded when selected
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(entityManager.Size()))
    {
        DrawFullEntityInspector(entityManager, selectedIndex);
    }

    ImGui::End();

    // Popups
    DrawTextureAssignmentPopup(entityManager, selectedIndex);
    DrawModelErrorPopup();
}

void EntityManagerInspector::DrawSpawnControls(EntityManager& entityManager, Vec3& spawnPosition,
                                              Vec3& spawnScale, int selectedIndex, bool useSharedCube)
{
    // Spawn position
    ImGui::Text("Spawn Position");
    ImGui::DragFloat3("##SpawnPos", &spawnPosition.x, 0.1f);

    // Scale
    ImGui::Text("Scale");
    ImGui::DragFloat3("##SpawnScale", &spawnScale.x, 0.01f, 0.001f, 100.0f);

    // Model path
    ImGui::Text("Model Path");
    ImGui::InputText("##ModelPath", m_modelPath, IM_ARRAYSIZE(m_modelPath));
    ImGui::SameLine();
    if (ImGui::Button("Browse..."))
    {
        char szFile[260] = {0};
        if (Platform::OpenFileDialog(szFile, sizeof(szFile),
            "3D Models\0*.obj;*.gltf;*.glb\0OBJ Files\0*.obj\0GLTF Files\0*.gltf;*.glb\0All\0*.*\0"))
        {
            strncpy_s(m_modelPath, szFile, sizeof(m_modelPath));
        }
    }

    // Spawn buttons
    if (ImGui::Button("Spawn Entity"))
    {
        SpawnNewEntity(entityManager, spawnPosition, spawnScale, useSharedCube);
    }
    ImGui::SameLine();
    if (ImGui::Button("Apply to Selected"))
    {
        ApplyModelToSelected(entityManager, selectedIndex);
    }

    // Options
    ImGui::Checkbox("Use shared cube mesh", &useSharedCube);
}

void EntityManagerInspector::SpawnNewEntity(EntityManager& entityManager, const Vec3& spawnPosition,
                                           const Vec3& spawnScale, bool useSharedCube)
{
    Entity entity;
    entity.name = "Cube";
    entity.Transform.Position = spawnPosition;
    entity.Transform.Scale = spawnScale;

    if (m_modelPath[0] != '\0')
    {
        std::string pathStr(m_modelPath);
        MeshHandle handle = MeshManager::Instance().LoadMeshSync(pathStr);
        if (handle != 0)
        {
            entity.MeshHandle = handle;
            entity.MeshPath = pathStr;
            entity.name = "Model: " + pathStr;
            entityManager.AddEntity(entity, useSharedCube);
        }
        else
        {
            m_showModelError = true;
            snprintf(m_modelErrorMsg, sizeof(m_modelErrorMsg), "Failed to load model: %s", m_modelPath);
        }
    }
    else
    {
        entityManager.AddEntity(entity, useSharedCube);
    }
}

void EntityManagerInspector::ApplyModelToSelected(EntityManager& entityManager, int selectedIndex)
{
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(entityManager.Size()))
    {
        return;
    }

    if (m_modelPath[0] == '\0')
    {
        return;
    }

    auto& entity = entityManager.GetAll()[selectedIndex];

    // Release old mesh
    if (entity.MeshHandle != 0)
    {
        MeshManager::Instance().Release(entity.MeshHandle);
    }

    // Load and assign new mesh
    MeshHandle handle = MeshManager::Instance().LoadMeshSync(m_modelPath);
    if (handle != 0)
    {
        entity.MeshHandle = handle;
        entity.MeshPath = std::string(m_modelPath);
        entity.name = "Model: " + std::string(m_modelPath);
    }
    else
    {
        m_showModelError = true;
        snprintf(m_modelErrorMsg, sizeof(m_modelErrorMsg), "Failed to load model: %s", m_modelPath);
    }
}

void EntityManagerInspector::DrawEntityList(EntityManager& entityManager, int& selectedIndex)
{
    auto& entities = entityManager.GetAll();

    ImGui::BeginChild("EntityListScroll", ImVec2(0, 200), true);
    ImGui::Columns(2);
    ImGui::SetColumnWidth(1, 90.0f);

    for (size_t i = 0; i < entities.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));

        auto& entity = entities[i];
        bool isSelected = (selectedIndex == static_cast<int>(i));

        // Show entity name with icon and vertex count
        std::string displayName = "?? " + entity.name;
        if (entity.MeshHandle != 0)
        {
            Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
            if (mesh)
            {
                displayName = "?? " + entity.name;
            }
        }

        // Left column: selectable entity name
        if (ImGui::Selectable(displayName.c_str(), isSelected))
        {
            selectedIndex = static_cast<int>(i);
        }

        ImGui::NextColumn();

        // Right column: delete button
        ImGui::AlignTextToFramePadding();
        if (ImGui::SmallButton("Delete"))
        {
            entityManager.RemoveAt(i);
            if (selectedIndex == static_cast<int>(i))
            {
                selectedIndex = -1;
            }
            else if (selectedIndex > static_cast<int>(i))
            {
                selectedIndex -= 1;
            }
            ImGui::PopID();
            break;
        }

        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);
    ImGui::EndChild();
}

void EntityManagerInspector::DrawFullEntityInspector(EntityManager& entityManager, int selectedIndex)
{
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(entityManager.Size()))
    {
        return;
    }

    auto& entity = entityManager.GetAll()[selectedIndex];

    ImGui::Text("Selected: %s", entity.name.c_str());
    ImGui::Separator();

    // Name
    char nameBuf[128];
    strncpy_s(nameBuf, entity.name.c_str(), sizeof(nameBuf));
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
    {
        entity.name = nameBuf;
    }

    // Transform
    ImGui::Spacing();
    ImGui::Text("Transform");
    ImGui::DragFloat3("Position", &entity.Transform.Position.x, 0.1f);
    ImGui::DragFloat3("Rotation", &entity.Transform.Rotation.x, 1.0f);
    ImGui::DragFloat3("Scale", &entity.Transform.Scale.x, 0.01f);

    // Mesh section
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Mesh");
    if (entity.MeshHandle != 0)
    {
        Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
        if (mesh)
        {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Loaded: [Cube]");
            ImGui::Text("Vertices: %zu", mesh->Vertices.size());

            if (ImGui::Button("Change Mesh"))
            {
                char buf[1024] = {0};
                if (Platform::OpenFileDialog(buf, sizeof(buf),
                    "3D Models\0*.obj;*.gltf;*.glb\0OBJ Files\0*.obj\0GLTF Files\0*.gltf;*.glb\0All\0*.*\0"))
                {
                    MeshHandle newHandle = MeshManager::Instance().LoadMeshSync(buf);
                    if (newHandle != 0)
                    {
                        if (entity.MeshHandle != 0)
                        {
                            MeshManager::Instance().Release(entity.MeshHandle);
                        }
                        entity.MeshHandle = newHandle;
                        entity.MeshPath = buf;
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove Mesh"))
            {
                MeshManager::Instance().Release(entity.MeshHandle);
                entity.MeshHandle = 0;
                entity.MeshPath = "";
            }
        }
    }
    else
    {
        ImGui::Text("No mesh assigned");
    }

    // Material Properties
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Material Properties");
    ImGui::SliderFloat("Shininess", &entity.Shininess, 1.0f, 256.0f);
    ImGui::SliderFloat("Alpha", &entity.Alpha, 0.0f, 1.0f);

    // Textures
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Textures");
    
    // Diffuse
    if (entity.HasDiffuseTextureOverride)
    {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Diffuse: Override Active");
        ImGui::Text("  %s", entity.DiffuseTexturePath.c_str());
        if (ImGui::Button("Remove Override##Diffuse"))
        {
            if (entity.DiffuseTexture != 0)
            {
                glDeleteTextures(1, &entity.DiffuseTexture);
                entity.DiffuseTexture = 0;
            }
            entity.HasDiffuseTextureOverride = false;
            entity.DiffuseTexturePath = "";
        }
    }
    else
    {
        ImGui::Text("Diffuse: (using mesh default)");
        if (ImGui::Button("Set Override##Diffuse"))
        {
            char buf[1024] = {0};
            if (Platform::OpenFileDialog(buf, sizeof(buf),
                "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All\0*.*\0"))
            {
                // Load texture
                int width, height, channels;
                unsigned char* data = stbi_load(buf, &width, &height, &channels, 4);
                if (data)
                {
                    unsigned int tex;
                    glGenTextures(1, &tex);
                    glBindTexture(GL_TEXTURE_2D, tex);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    stbi_image_free(data);
                    
                    entity.DiffuseTexture = tex;
                    entity.DiffuseTexturePath = buf;
                    entity.HasDiffuseTextureOverride = true;
                }
            }
        }
    }

    ImGui::Spacing();

    // Specular
    if (entity.HasSpecularTextureOverride)
    {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Specular: Override Active");
        ImGui::Text("  %s", entity.SpecularTexturePath.c_str());
        if (ImGui::Button("Remove Override##Specular"))
        {
            if (entity.SpecularTexture != 0)
            {
                glDeleteTextures(1, &entity.SpecularTexture);
                entity.SpecularTexture = 0;
            }
            entity.HasSpecularTextureOverride = false;
            entity.SpecularTexturePath = "";
        }
    }
    else
    {
        ImGui::Text("Specular: (using mesh default)");
        if (ImGui::Button("Set Override##Specular"))
        {
            char buf[1024] = {0};
            if (Platform::OpenFileDialog(buf, sizeof(buf),
                "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All\0*.*\0"))
            {
                // Load texture
                int width, height, channels;
                unsigned char* data = stbi_load(buf, &width, &height, &channels, 1);
                if (data)
                {
                    unsigned int tex;
                    glGenTextures(1, &tex);
                    glBindTexture(GL_TEXTURE_2D, tex);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    stbi_image_free(data);
                    
                    entity.SpecularTexture = tex;
                    entity.SpecularTexturePath = buf;
                    entity.HasSpecularTextureOverride = true;
                }
            }
        }
    }

    ImGui::Spacing();

    // Normal
    if (entity.HasNormalTextureOverride)
    {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Normal: Override Active");
        ImGui::Text("  %s", entity.NormalTexturePath.c_str());
        if (ImGui::Button("Remove Override##Normal"))
        {
            if (entity.NormalTexture != 0)
            {
                glDeleteTextures(1, &entity.NormalTexture);
                entity.NormalTexture = 0;
            }
            entity.HasNormalTextureOverride = false;
            entity.NormalTexturePath = "";
        }
    }
    else
    {
        ImGui::Text("Normal: (using mesh default)");
        if (ImGui::Button("Set Override##Normal"))
        {
            char buf[1024] = {0};
            if (Platform::OpenFileDialog(buf, sizeof(buf),
                "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All\0*.*\0"))
            {
                // Load texture
                int width, height, channels;
                unsigned char* data = stbi_load(buf, &width, &height, &channels, 4);
                if (data)
                {
                    unsigned int tex;
                    glGenTextures(1, &tex);
                    glBindTexture(GL_TEXTURE_2D, tex);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    stbi_image_free(data);
                    
                    entity.NormalTexture = tex;
                    entity.NormalTexturePath = buf;
                    entity.HasNormalTextureOverride = true;
                }
            }
        }
    }
}

void EntityManagerInspector::DrawTextureAssignmentPopup(EntityManager& entityManager, int selectedIndex)
{
    // Check if browsed file is a texture
    if (m_modelPath[0] != '\0')
    {
        std::string path(m_modelPath);
        std::string ext;
        auto pos = path.find_last_of('.');
        if (pos != std::string::npos)
        {
            ext = path.substr(pos + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(),
                         [](unsigned char c) { return std::tolower(c); });
        }

        if ((ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp") && selectedIndex >= 0)
        {
            m_pendingTexturePath = path;
            ImGui::OpenPopup("AssignTexturePopup");
            m_modelPath[0] = '\0';
        }
    }

    if (ImGui::BeginPopup("AssignTexturePopup"))
    {
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(entityManager.Size()))
        {
            auto& entity = entityManager.GetAll()[selectedIndex];

            if (ImGui::MenuItem("Diffuse"))
            {
                if (entity.MeshHandle != 0)
                {
                    Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
                    if (mesh)
                    {
                        mesh->LoadTexture(m_pendingTexturePath);
                        mesh->DiffuseTexturePath = m_pendingTexturePath;
                    }
                }
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Specular"))
            {
                if (entity.MeshHandle != 0)
                {
                    Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
                    if (mesh)
                    {
                        mesh->LoadSpecularTexture(m_pendingTexturePath);
                        mesh->SpecularTexturePath = m_pendingTexturePath;
                    }
                }
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Normal"))
            {
                if (entity.MeshHandle != 0)
                {
                    Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
                    if (mesh)
                    {
                        mesh->LoadNormalTexture(m_pendingTexturePath);
                        mesh->NormalTexturePath = m_pendingTexturePath;
                    }
                }
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }
}

void EntityManagerInspector::DrawModelErrorPopup()
{
    if (m_showModelError)
    {
        ImGui::OpenPopup("Model Load Error");
        if (ImGui::BeginPopupModal("Model Load Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(m_modelErrorMsg);
            if (ImGui::Button("OK"))
            {
                ImGui::CloseCurrentPopup();
                m_showModelError = false;
            }
            ImGui::EndPopup();
        }
    }
}
