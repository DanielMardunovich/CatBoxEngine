#include "SpawnInspector.h"
#include "../../core/Platform.h"
#include "../../resources/EntityManager.h"
#include "../../resources/Entity.h"
#include "../../graphics/MeshManager.h"
#include "../../graphics/Mesh.h"
#include "imgui.h"
#include <algorithm>
#include <cctype>

void SpawnInspector::Draw(EntityManager& entityManager, Vec3& spawnPosition, Vec3& spawnScale,
                         int selectedIndex, bool& useSharedCube)
{
    ImGui::Begin("Spawn Entity");

    ImGui::Text("Entities: %zu", entityManager.Size());
    ImGui::Separator();

    // Spawn position controls
    ImGui::Text("Spawn Position");
    ImGui::DragFloat3("##Position", &spawnPosition.x, 0.1f);

    // Spawn scale controls
    ImGui::Text("Scale");
    ImGui::DragFloat3("##Scale", &spawnScale.x, 0.01f, 0.001f, 100.0f);

    ImGui::Separator();

    // Model Path
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

    ImGui::Separator();

    // Spawn buttons in a row (like Light Inspector)
    if (ImGui::Button("Spawn Entity"))
    {
        SpawnNewEntity(entityManager, spawnPosition, spawnScale, useSharedCube);
    }

    ImGui::SameLine();

    if (ImGui::Button("Apply to Selected"))
    {
        ApplyModelToSelected(entityManager, selectedIndex);
    }

    ImGui::Separator();

    // Options
    ImGui::Checkbox("Use shared cube mesh", &useSharedCube);

    ImGui::End();

    // Handle texture assignment popup
    DrawTextureAssignmentPopup(entityManager, selectedIndex);

    // Error Popup
    DrawModelErrorPopup();
}

void SpawnInspector::SpawnNewEntity(EntityManager& entityManager, const Vec3& spawnPosition,
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

void SpawnInspector::ApplyModelToSelected(EntityManager& entityManager, int selectedIndex)
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
    
    // Release old mesh handle
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

void SpawnInspector::DrawTextureAssignmentPopup(EntityManager& entityManager, int selectedIndex)
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
            m_modelPath[0] = '\0'; // Clear path after detecting texture
        }
    }

    // Texture assignment popup
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

void SpawnInspector::DrawModelErrorPopup()
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
